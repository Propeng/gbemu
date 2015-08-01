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
#include <SFML/Window/Joystick.hpp>
#include "EmulatorWidget.h"
#include "GBWindow.h"
#include "AudioBuffer.h"
#include "PrinterPreview.h"
#include "settings.h"
extern "C" {
	#include "emu/gb.h"
	#include "emu/rom.h"
	#include "emu/video/video.h"
	#include "emu/mbc/mbc.h"
	//#include "sdl/audio.h"
}

QMutex EmulatorWidget::audio_mutex;

EmulatorWidget::EmulatorWidget(UserSettings *user_settings) : QOpenGLWidget() {
	this->user_settings = user_settings;
	//audio_file = fopen("samples", "wb");
	audio_buf = NULL;
	printerBuf = NULL;
	callbuf_data = NULL;
	loaded = false;
	overlay_msg[0] = '\0';
	overlay_time = 0;
	window_inactive = 0;

	gb = init_context();
	apply_gb_settings();
	gb->settings.sample_rate = 48000;
	gb->settings.boot_rom = load_boot_roms();
	gb->settings.save_ram = save_ram_callback;
	gb->settings.play_sound = write_audio;
	gb->settings.callback_data = this;
	init_audio();

	last_time = 0;
	//last_sec = 0;
	elapsed = new QElapsedTimer();
	elapsed->start();

	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &EmulatorWidget::timer_tick);
	timer->start(10);
	
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
	resize(DISPLAY_WIDTH*2, DISPLAY_HEIGHT*2);
	setMinimumSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
	updateGeometry();

	//setUpdatesEnabled(false);
	run_flag = false;
}

void EmulatorWidget::apply_gb_settings() {
	gb->settings.cgb_hw = user_settings->cgb_hw;
	gb->settings.sgb_hw = user_settings->sgb_hw;
	gb->settings.dmg_hw = user_settings->dmg_hw;
	immediate_settings();
}

void EmulatorWidget::immediate_settings() {
	memcpy(gb->settings.dmg_palette, user_settings->dmg_palette, sizeof(gb->settings.dmg_palette));
	gb->settings.emulate_lcd = user_settings->emulate_lcd;
}

bool EmulatorWidget::load_boot_roms() {
	FILE *dmgrom = fopen("dmgboot.bin", "rb");
	if (dmgrom == NULL) return false;
	fread(gb->dmg_bootrom, 1, sizeof(gb->dmg_bootrom), dmgrom);
	fclose(dmgrom);
	
	FILE *sgbrom = fopen("sgbboot.bin", "rb");
	if (sgbrom == NULL) return false;
	fread(gb->sgb_bootrom, 1, sizeof(gb->sgb_bootrom), sgbrom);
	fclose(sgbrom);
	
	FILE *cgbrom = fopen("cgbboot.bin", "rb");
	if (cgbrom == NULL) return false;
	fread(gb->cgb_bootrom, 1, sizeof(gb->cgb_bootrom), cgbrom);
	fclose(cgbrom);

	return true;
}

bool EmulatorWidget::load_rom_file(const char* filename) {
	if (loaded && gb->has_battery) save_ram();
	loaded = false;
	
	apply_gb_settings();

	FILE *rom = fopen(filename, "rb");
	if (rom == NULL) return false;
	fseek(rom, 0, SEEK_END);
	size_t len = ftell(rom);
	fseek(rom, 0, SEEK_SET);
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
		if (user_settings->skip_bootrom) skip_bootrom(gb);

		//audio_buf->reset();
		//unpause_audio();
		loaded = true;
	} else {
		//audio_buf->reset();
		//pause_audio();
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
	fseek(save_file, 0, SEEK_SET);

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
	if (loaded && gb->has_battery) save_ram();
	apply_gb_settings();
	reset_gb(gb);
	if (gb->has_battery) load_ram();
	if (user_settings->skip_bootrom) skip_bootrom(gb);
	gb->settings.paused = 0;
}

QSize EmulatorWidget::sizeHint() const {
	return QSize(DISPLAY_WIDTH*2, DISPLAY_HEIGHT*2);
}

void EmulatorWidget::init_audio() {
	/*audio_buf = new AudioBuffer(SND_BUFLEN*sizeof(float));
	audio_buf->open(QIODevice::ReadWrite | QIODevice::Unbuffered);
	init_sdl_audio(gb->settings.sample_rate, N_CHANNELS, SND_BUFLEN/N_CHANNELS, &sdl_callback, this);*/
	audio_buf = new AudioBuffer(SND_BUFLEN*sizeof(int16_t), 2, gb->settings.sample_rate);
	
	audio_buf->play();
}

/*void EmulatorWidget::sdl_callback(void *data, Uint8 *stream, int len) {
	EmulatorWidget *widget = (EmulatorWidget*)data;
	memset(stream, 0, len);
	QMutexLocker lock(&EmulatorWidget::audio_mutex);
	if (widget->audio_buf != NULL && widget->audio_buf->isOpen()) {
		if (widget->audio_buf->bytesAvailable() < len) return;
		int read_bytes = 0;
		while (read_bytes < len) {
			read_bytes += widget->audio_buf->read((char*)stream + read_bytes, len - read_bytes);
		}
		if (!widget->user_settings->enable_sound) {
			memset(stream, 0, len);
		}
	}
}*/

void EmulatorWidget::write_audio(int16_t *samples, int n_bytes, void *data) {
	EmulatorWidget *widget = (EmulatorWidget*)data;
	QMutexLocker lock(&EmulatorWidget::audio_mutex);
	/*//fwrite(samples, 1, n_bytes, widget->audio_file);
	//fflush(widget->audio_file);
	if (widget->audio_buf != NULL && widget->audio_buf->isOpen()) {
		widget->audio_buf->write((char*)samples, n_bytes);
	}*/
	widget->audio_buf->write(samples, n_bytes);
}

/*void EmulatorWidget::paintEvent(QPaintEvent *paintEvent) {
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
}*/

void EmulatorWidget::initializeGL() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	void *buf = malloc(DISPLAY_WIDTH*DISPLAY_HEIGHT*4);
	memset(buf, 0, DISPLAY_WIDTH*DISPLAY_HEIGHT*4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, buf);
	free(buf);
}

void EmulatorWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);
	glLoadIdentity();
}

void EmulatorWidget::paintGL() {
	glClear(GL_COLOR_BUFFER_BIT);
	
	bool flag = run_flag;
	run_flag = false;

	if (loaded && flag && !gb->settings.paused) {
		update_joystick();
		uint32_t *framebuf = run_frame(gb);
		if (framebuf != NULL) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, framebuf);
		}
	}
	
	if (loaded) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(0, 0);
		glTexCoord2f(0, 1);
		glVertex2f(0, DISPLAY_HEIGHT);
		glTexCoord2f(1, 1);
		glVertex2f(DISPLAY_WIDTH, DISPLAY_HEIGHT);
		glTexCoord2f(1, 0);
		glVertex2f(DISPLAY_WIDTH, 0);
		glEnd();
	}

	if (last_time < overlay_time) {
		QPainter painter(this);
		painter.setRenderHints(QPainter::Antialiasing);
		QFont font("Helvetica", 10, QFont::Bold);
		painter.setFont(font);
		QFontMetrics fm(font);
		float fade = overlay_time-last_time < 500 ? (overlay_time-last_time)/500 : 1;
		painter.setPen(QPen(QColor(255, 255, 255, fade*255)));
		painter.drawText(6, height()-fm.height()-4, width()-5, fm.height()+5, Qt::AlignTop | Qt::AlignLeft, overlay_msg);
		painter.setPen(QPen(QColor(210, 0, 0, fade*255)));
		painter.drawText(5, height()-fm.height()-5, width()-5, fm.height()+5, Qt::AlignTop | Qt::AlignLeft, overlay_msg);
	}
}

void EmulatorWidget::update_joystick() {
	int gbbtn;
	sf::Joystick::update();
	for (int i = 0; i < 8; i++) {
		if (user_settings->bindings[i].device != -1) {
			if (sf::Joystick::isConnected(user_settings->bindings[i].device)) {
				switch (i) {
				case KEYBIND_UP: gbbtn = BTN_UP; break;
				case KEYBIND_DOWN: gbbtn = BTN_DOWN; break;
				case KEYBIND_LEFT: gbbtn = BTN_LEFT; break;
				case KEYBIND_RIGHT: gbbtn = BTN_RIGHT; break;
				case KEYBIND_START: gbbtn = BTN_START; break;
				case KEYBIND_SELECT: gbbtn = BTN_SELECT; break;
				case KEYBIND_A: gbbtn = BTN_A; break;
				case KEYBIND_B: gbbtn = BTN_B; break;
				}

				if (user_settings->bindings[i].type == KEYTYPE_BTN) {
					key_state(gb, gbbtn, sf::Joystick::isButtonPressed(user_settings->bindings[i].device,
						user_settings->bindings[i].key) ? BTN_PRESSED : BTN_UNPRESSED);
				} else if (user_settings->bindings[i].type == KEYTYPE_AXISPOS) {
					key_state(gb, gbbtn, sf::Joystick::getAxisPosition(user_settings->bindings[i].device,
						(sf::Joystick::Axis)user_settings->bindings[i].key) > 25 ? BTN_PRESSED : BTN_UNPRESSED);
				} else if (user_settings->bindings[i].type == KEYTYPE_AXISNEG) {
					key_state(gb, gbbtn, sf::Joystick::getAxisPosition(user_settings->bindings[i].device,
						(sf::Joystick::Axis)user_settings->bindings[i].key) < -25 ? BTN_PRESSED : BTN_UNPRESSED);
				}
			}
		}
	}
}

int EmulatorWidget::gb_button(int key) {
	int btn = 0;
	if (key == user_settings->bindings[KEYBIND_UP].key && user_settings->bindings[KEYBIND_UP].device == -1)
		btn |= BTN_UP;
	if (key == user_settings->bindings[KEYBIND_DOWN].key && user_settings->bindings[KEYBIND_DOWN].device == -1)
		btn |= BTN_DOWN;
	if (key == user_settings->bindings[KEYBIND_LEFT].key && user_settings->bindings[KEYBIND_LEFT].device == -1)
		btn |= BTN_LEFT;
	if (key == user_settings->bindings[KEYBIND_RIGHT].key && user_settings->bindings[KEYBIND_RIGHT].device == -1)
		btn |= BTN_RIGHT;
	if (key == user_settings->bindings[KEYBIND_START].key && user_settings->bindings[KEYBIND_START].device == -1)
		btn |= BTN_START;
	if (key == user_settings->bindings[KEYBIND_SELECT].key && user_settings->bindings[KEYBIND_SELECT].device == -1)
		btn |= BTN_SELECT;
	if (key == user_settings->bindings[KEYBIND_A].key && user_settings->bindings[KEYBIND_A].device == -1)
		btn |= BTN_A;
	if (key == user_settings->bindings[KEYBIND_B].key && user_settings->bindings[KEYBIND_B].device == -1)
		btn |= BTN_B;
	return btn;
}

void EmulatorWidget::keyPressEvent(QKeyEvent *keyEvent) {
	int btn = gb_button(keyEvent->key());
	if (btn != 0) {
		key_state(gb, btn, BTN_PRESSED);
	}
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

		if (!window_inactive || !user_settings->pause_unfocus) {
			//setUpdatesEnabled(true);
			run_flag = true;
			update();
		}
	}
}

void EmulatorWidget::closeEvent(QCloseEvent *closeEvent) {
	if (loaded) {
		if (printerBuf != NULL) show_printer_buf(true);
		if (gb->has_battery) save_ram();
	}
	gb->settings.play_sound = NULL;
	gb->settings.save_ram = NULL;

	//pause_audio();
	//SDL_LockAudio();
	//audio_buf->close();
	delete audio_buf;
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

void EmulatorWidget::show_printer_buf(bool disableContinue) {
	int oldpause = gb->settings.paused;
	gb->settings.paused = 1;
	if (printerBuf == NULL) {
		QMessageBox msg(QMessageBox::Information, parentWidget()->windowTitle(), "Printer buffer is empty.", QMessageBox::Ok);
		msg.exec();
	} else {
		PrinterPreview preview(&printerBuf, disableContinue);
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
	if (printerBuf != NULL) show_printer_buf(true);
	init_peripheral(&gb->peripheral, DEVICE_NONE, NULL, NULL);
}

void EmulatorWidget::periph_printer() {
	init_peripheral(&gb->peripheral, DEVICE_PRINTER, printer_callback, this);
}