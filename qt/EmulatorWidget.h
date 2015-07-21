#ifndef EMULATORWIDGET_H
#define EMULATORWIDGET_H

#include <QtWidgets/qdesktopwidget.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qtimer.h>
#include <QtCore/qelapsedtimer.h>
#include "AudioBuffer.h"
extern "C" {
	#include "emu/gb.h"
	#include "sdl/audio.h"
}

class EmulatorWidget : public QWidget
{
public:
	GBContext *gb;
	EmulatorWidget();
	bool load_rom_file(const char* filename);
	QSize sizeHint() const;
	static void write_audio(float *samples, int n_bytes, void *data);
	static void sdl_callback(void *data, Uint8 *stream, int len);
	static void save_ram_callback(void *data);
	static void printer_callback(GBPeripheralDevice device, void *data, int width, int height, void *user_data);
	void toggle_pause();
	void reset();
	void load_ram();
	void save_ram();
	void load_state();
	void save_state();
	QImage *printerBuf;
	
	void *callbuf_data;
	int callbuf_width;
	int callbuf_height;
	void printer_preview(void *data, int width, int height);
	
	void periph_disconnect();
	void periph_printer();
	void show_printer_buf(bool allowContinue = true);

protected:
	void paintEvent(QPaintEvent *paintEvent);
	void keyPressEvent(QKeyEvent *keyEvent);
	void keyReleaseEvent(QKeyEvent *keyEvent);
	void closeEvent(QCloseEvent *closeEvent);

private:
	bool run_flag;
	bool loaded;
	float last_time;
	//int last_sec;
	QTimer *timer;
	bool load_boot_roms();
	QElapsedTimer *elapsed;
	AudioBuffer *audio_buf;
	void init_audio();
	void timer_tick();
	int gb_button(int key);
	FILE *audio_file;

	char overlay_msg[1024];
	int overlay_time;
	void show_msg(const char *format, ...);
};

#endif