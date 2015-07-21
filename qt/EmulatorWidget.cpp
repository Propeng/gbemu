#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cerrno>
#include <QtWidgets/qwidget.h>
#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>
#include <QtGui/qimage.h>
#include <QtWidgets/qmessagebox.h>
#include <QtCore/qtimer.h>
#include <QtCore/qelapsedtimer.h>
#include "EmulatorWidget.h"
#include "GBWindow.h"
#include "AudioBuffer.h"
#include "PrinterPreview.h"
extern "C" {
	#include "emu/gb.h"
	#include "emu/rom/rom.h"
	#include "emu/video/video.h"
	#include "emu/mbc/mbc.h"
	#include "sdl/audio.h"
}

EmulatorWidget::EmulatorWidget() : QWidget() {
	//audio_file = fopen("samples", "wb");
	audio_buf = NULL;
	printerBuf = NULL;
	callbuf_data = NULL;
	loaded = false;

	gb = init_context();
	gb->settings.sample_rate = 48000;
	gb->settings.emulate_lcd = 1;
	gb->settings.boot_rom = load_boot_roms();
	//gb->settings.hw_type = GB_FORCE_CGB;
	gb->settings.save_ram = save_ram_callback;
	gb->settings.play_sound = write_audio;
	gb->settings.callback_data = this;
	//load_rom_file("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Pokemon - Crystal Version (USA, Europe).gbc");
	init_audio();

	last_time = 0;
	//last_sec = 0;
	elapsed = new QElapsedTimer();
	elapsed->start();

	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &EmulatorWidget::timer_tick);
	timer->start(10);
	
	setFocusPolicy(Qt::StrongFocus);
	resize(DISPLAY_WIDTH*2, DISPLAY_HEIGHT*2);
	updateGeometry();

	//setUpdatesEnabled(false);
	run_flag = false;
}

bool EmulatorWidget::load_boot_roms() {
	FILE *dmgrom = fopen("dmgboot.bin", "rb");
	if (dmgrom == NULL) return false;
	fread(gb->dmg_bootrom, 1, sizeof(gb->dmg_bootrom), dmgrom);
	fclose(dmgrom);
	
	FILE *cgbrom = fopen("cgbboot.bin", "rb");
	if (cgbrom == NULL) return false;
	fread(gb->cgb_bootrom, 1, sizeof(gb->cgb_bootrom), cgbrom);
	fclose(cgbrom);

	return true;
}

bool EmulatorWidget::load_rom_file(const char* filename) {
	if (loaded && gb->has_battery) save_ram();

	FILE *rom = fopen(filename, "rb");
	fseek(rom, 0, SEEK_END);
	size_t len = ftell(rom);
	fseek(rom, 0, 0);
	uint8_t *buf = (uint8_t*)malloc(len);
	fread(buf, 1, len, rom);
	fclose(rom);
	
	bool run;
	if (load_rom(gb, buf, len)) {
		//init_peripheral(&gb->peripheral, DEVICE_PRINTER, &printer_callback, this);
		run = true;
	} else {
		char msg[100];
		sprintf(msg, "Unsupported cartridge type %02X. Attempt to run anyway?", gb->cartridge_type);
		QMessageBox *msgbox = new QMessageBox(QMessageBox::Question, parentWidget()->windowTitle(), msg, QMessageBox::Yes | QMessageBox::No, this);
		run = msgbox->exec() == QMessageBox::Yes;
	}

	if (run) {
		if (gb->has_battery) load_ram();

		audio_buf->reset();
		unpause_audio();
		loaded = true;
	} else {
		audio_buf->reset();
		pause_audio();
		loaded = false;
	}
	gb->error = 0;
	free(buf);
	return run;
}

void EmulatorWidget::load_ram() {
	FILE *save_file;
	GBBuffer buf = { 0 };
	char filename[1024];
	sprintf(filename, "%s.sav", gb->rom_title_safe);

	save_file = fopen(filename, "rb");
	if (save_file == NULL) {
		printf("-- Failed to read save file %s: %s\n", filename, strerror(errno));
		return;
	}
	
	fseek(save_file, 0, SEEK_END);
	buf.data_len = ftell(save_file);
	fseek(save_file, 0, 0);

	buf.data = (uint8_t*)malloc(buf.data_len);
	fread(buf.data, 1, buf.data_len, save_file);
	fclose(save_file);
	ram_load(gb, &buf);
	free(buf.data);
	
	printf("-- Loaded save from %s.\n", filename);
	show_msg("Loaded save from %s.\n", filename);
}

void EmulatorWidget::save_ram() {
	FILE *save_file;
	GBBuffer buf = { 0 };
	char filename[1024];
	sprintf(filename, "%s.sav", gb->rom_title_safe);
	
	save_file = fopen(filename, "wb");
	if (save_file == NULL) {
		printf("-- Failed to open file %s for saving: %s\n", filename, strerror(errno));
		show_msg("Failed to write save file.");
		return;
	}
	fseek(save_file, 0, SEEK_SET);
	ram_dump(gb, &buf);
	fwrite(buf.data, 1, buf.data_len, save_file);
	if (buf.rtc_len > 0) fwrite(buf.rtc_data, 1, buf.rtc_len, save_file);
	fflush(save_file);
	fclose(save_file);

	printf("-- Saved to %s.\n", filename);
}

void EmulatorWidget::load_state() {
	FILE *save_file;
	char filename[1024];
	sprintf(filename, "%s.state", gb->rom_title_safe);

	save_file = fopen(filename, "rb");
	if (save_file == NULL) {
		printf("-- Failed to read save file %s: %s\n", filename, strerror(errno));
		show_msg("State file not found.");
		return;
	}
	
	uint8_t *rom = gb->rom;
	GBSettings temp_settings;
	GBPeripheralInfo temp_periph;
	memcpy(&temp_settings, &gb->settings, sizeof(GBSettings));
	memcpy(&temp_periph, &gb->peripheral, sizeof(GBPeripheralInfo));
	fread(gb, sizeof(GBContext), 1, save_file);
	mbc_init(gb);
	fread(gb->mbc_context, mbc_size(gb), 1, save_file);
	memcpy(&gb->settings, &temp_settings, sizeof(GBSettings));
	memcpy(&gb->peripheral, &temp_periph, sizeof(GBPeripheralInfo));
	gb->last_framebuf = NULL;
	gb->rom = rom;

	fclose(save_file);
	printf("-- Loaded state from %s.\n", filename);
	show_msg("Loaded state from %s.", filename);
}

void EmulatorWidget::save_state() {
	FILE *save_file;
	char filename[1024];
	sprintf(filename, "%s.state", gb->rom_title_safe);
	
	save_file = fopen(filename, "wb");
	if (save_file == NULL) {
		printf("-- Failed to open file %s for saving: %s\n", filename, strerror(errno));
		show_msg("Failed to write save file.");
		return;
	}
	fseek(save_file, 0, SEEK_SET);
	fwrite(gb, sizeof(GBContext), 1, save_file);
	if (gb->mbc_context != NULL) fwrite(gb->mbc_context, mbc_size(gb), 1, save_file);
	fflush(save_file);
	fclose(save_file);

	printf("-- Saved state to %s.\n", filename);
	show_msg("Saved state to %s.", filename);
}

void EmulatorWidget::save_ram_callback(void *data) {
	EmulatorWidget *widget = (EmulatorWidget*)data;
	widget->save_ram();
}

void EmulatorWidget::toggle_pause() {
	gb->settings.paused = !gb->settings.paused;
}

void EmulatorWidget::reset() {
	reset_gb(gb);
	gb->settings.paused = 0;
}

QSize EmulatorWidget::sizeHint() const {
	return QSize(DISPLAY_WIDTH*2, DISPLAY_HEIGHT*2);
}

void EmulatorWidget::init_audio() {
	audio_buf = new AudioBuffer(SND_BUFLEN*sizeof(float));
	audio_buf->open(QIODevice::ReadWrite);
	init_sdl_audio(gb->settings.sample_rate, N_CHANNELS, SND_BUFLEN/N_CHANNELS, &sdl_callback, this);
}

void EmulatorWidget::sdl_callback(void *data, Uint8 *stream, int len) {
	EmulatorWidget *widget = (EmulatorWidget*)data;
	memset(stream, 0, len);
	if (widget->audio_buf != NULL && widget->audio_buf->isOpen()) {
		if (widget->audio_buf->bytesAvailable() == 0) return;
		int read_bytes = 0;
		while (read_bytes < len) {
			read_bytes += widget->audio_buf->read((char*)stream + read_bytes, len - read_bytes);
		}
	}
}

void EmulatorWidget::write_audio(float *samples, int n_bytes, void *data) {
	EmulatorWidget *widget = (EmulatorWidget*)data;
	//fwrite(samples, 1, n_bytes, widget->audio_file);
	//fflush(widget->audio_file);
	if (widget->audio_buf != NULL && widget->audio_buf->isOpen()) {
		widget->audio_buf->write((char*)samples, n_bytes);
	}
}

void EmulatorWidget::paintEvent(QPaintEvent *paintEvent) {
	//setUpdatesEnabled(false);
	bool flag = run_flag;
	run_flag = false;

	QPainter painter(this);
	painter.fillRect(0, 0, width(), height(), Qt::black);

	if (loaded) {
		uint32_t *framebuf = flag ? run_frame(gb) : gb->last_framebuf;
		if (framebuf != NULL) {
			QImage image((uchar*)framebuf, DISPLAY_WIDTH, DISPLAY_HEIGHT, QImage::Format_RGB32);
			painter.drawImage(QRect(0, 0, width(), height()), image);
		}
	}

	if (last_time < overlay_time) {
		QFont font("Helvetica", 10, QFont::Bold);
		painter.setFont(font);
		float fade = overlay_time-last_time < 500 ? (overlay_time-last_time)/500 : 1;
		painter.setPen(QPen(QColor(255, 0, 0, fade*255)));
		QFontMetrics fm(font);
		painter.drawText(5, height()-fm.height()-5, width()-5, fm.height()+5, Qt::AlignTop | Qt::AlignLeft, overlay_msg);
	}

	/*if (gb->frame_counter % FRAMES_PER_SEC == 0) {
		int now = elapsed->elapsed();
		printf("%u frames in %u ms, %f fps\n", gb->frame_counter, now-last_sec, (float)FRAMES_PER_SEC/((float)(now-last_sec)/1000));
		last_sec = now;
	}*/
}

int EmulatorWidget::gb_button(int key) {
	switch (key) {
	case Qt::Key_Up:
		return BTN_UP;
	case Qt::Key_Down:
		return BTN_DOWN;
	case Qt::Key_Left:
		return BTN_LEFT;
	case Qt::Key_Right:
		return BTN_RIGHT;
	case Qt::Key_Space:
		return BTN_START;
	case Qt::Key_Control:
		return BTN_SELECT;
	case Qt::Key_X:
		return BTN_A;
	case Qt::Key_Z:
		return BTN_B;
	}
	return 0;
}

void EmulatorWidget::keyPressEvent(QKeyEvent *keyEvent) {
	int btn = gb_button(keyEvent->key());
	if (btn != 0) {
		key_state(gb, btn, BTN_PRESSED);
	}/* else if (loaded) {
		if (keyEvent->key() == Qt::Key_F1) ((GBWindow*)parentWidget())->toggle_pause();
		else if (keyEvent->key() == Qt::Key_F2) ((GBWindow*)parentWidget())->reset();
		else if (keyEvent->key() == Qt::Key_F3) save_state();
		else if (keyEvent->key() == Qt::Key_F4) load_state();
	}*/
}

void EmulatorWidget::keyReleaseEvent(QKeyEvent *keyEvent) {
	int btn = gb_button(keyEvent->key());
	if (btn != 0) {
		key_state(gb, btn, BTN_UNPRESSED);
	}
}

void EmulatorWidget::timer_tick() {
	float now = elapsed->elapsed();
	while (now - last_time >= 1000.f/FRAMES_PER_SEC) {
		last_time += 1000.f/FRAMES_PER_SEC;

		if (callbuf_data != NULL) {
			printer_preview(callbuf_data, callbuf_width, callbuf_height);
			free(callbuf_data);
			callbuf_data = NULL;
		}

		//setUpdatesEnabled(true);
		run_flag = true;
		update();
	}
}

void EmulatorWidget::closeEvent(QCloseEvent *closeEvent) {
	if (loaded) {
		if (printerBuf != NULL) show_printer_buf(false);
		if (gb->has_battery) save_ram();
	}
	gb->settings.play_sound = NULL;
	gb->settings.save_ram = NULL;

	pause_audio();
	SDL_LockAudio();
	audio_buf->close();
	audio_buf = NULL;
}

void EmulatorWidget::show_msg(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsprintf(overlay_msg, format, args);
	va_end(args);
	overlay_time = last_time + 2000;
}

void EmulatorWidget::printer_preview(void *data, int width, int height) {
	if (printerBuf == NULL) {
		void *data_copy = malloc(width*height*sizeof(uint32_t));
		memcpy(data_copy, data, width*height*sizeof(uint32_t));
		printerBuf = new QImage((uchar*)data_copy, width, height, QImage::Format_RGB32);
	} else {
		QImage image((uchar*)data, width, height, QImage::Format_RGB32);
		//QImage oldimage = printerBuf->copy();
		QImage *newPrinterBuf = new QImage(width, printerBuf->height()+height, QImage::Format_RGB32);

		QPainter painter;
		painter.begin(newPrinterBuf);
		painter.drawImage(QRect(0, 0, width, printerBuf->height()), *printerBuf);
		painter.drawImage(QRect(0, printerBuf->height(), width, height), image);
		painter.end();

		delete printerBuf;
		printerBuf = newPrinterBuf;
	}

	int oldpause = gb->settings.paused;
	gb->settings.paused = 1;
	PrinterPreview preview(&printerBuf);
	preview.exec();
	gb->settings.paused = oldpause;
	activateWindow();
}

void EmulatorWidget::show_printer_buf(bool allowContinue) {
	int oldpause = gb->settings.paused;
	gb->settings.paused = 1;
	if (printerBuf == NULL) {
		QMessageBox msg(QMessageBox::Information, parentWidget()->windowTitle(), "Printer buffer is empty.", QMessageBox::Ok);
		msg.exec();
	} else {
		PrinterPreview preview(&printerBuf, allowContinue);
		preview.exec();
	}
	gb->settings.paused = oldpause;
	activateWindow();
}

void EmulatorWidget::printer_callback(GBPeripheralDevice device, void *data, int width, int height, void *user_data) {
	EmulatorWidget *widget = (EmulatorWidget*)user_data;
	widget->callbuf_data = data;
	widget->callbuf_width = width;
	widget->callbuf_height = height;
}

void EmulatorWidget::periph_disconnect() {
	if (printerBuf != NULL) show_printer_buf();
	init_peripheral(&gb->peripheral, DEVICE_NONE, NULL, NULL);
}

void EmulatorWidget::periph_printer() {
	init_peripheral(&gb->peripheral, DEVICE_PRINTER, printer_callback, this);
}