#ifndef EMULATORWIDGET_H
#define EMULATORWIDGET_H

#include <QtWidgets/qopenglwidget.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qtimer.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmutex.h>
#include "AudioBuffer.h"
#include "settings.h"
extern "C" {
	#include "emu/gb.h"
	//#include "sdl/audio.h"
}

class EmulatorWidget : public QOpenGLWidget
{
public:
	static QMutex audio_mutex;
	GBContext *gb;
	QMainWindow *window;
	EmulatorWidget(QMainWindow *window, UserSettings *user_settings);
	bool load_rom_file(const char* filename);
	QSize sizeHint() const;
	static void write_audio(int16_t *samples, int n_bytes, void *data);
	//static void sdl_callback(void *data, Uint8 *stream, int len);
	static void save_ram_callback(void *data);
	static void printer_callback(GBPeripheralDevice device, void *data, int width, int height, void *user_data);
	static void frame_callback(uint32_t *frame, void *data);
	void toggle_pause();
	void reset();
	void load_ram();
	void save_ram();
	void load_state();
	void save_state();
	QImage *printerBuf;
	void immediate_settings();
	bool show_frame;

	int window_inactive;
	
	void *callbuf_data;
	int callbuf_width;
	int callbuf_height;
	void printer_preview(void *data, int width, int height);
	
	void periph_disconnect();
	void periph_printer();
	void show_printer_buf(bool disableContinue = false);
	void show_msg(const char *format, ...);

protected:
	//void paintEvent(QPaintEvent *paintEvent);
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();
	void keyPressEvent(QKeyEvent *keyEvent);
	void keyReleaseEvent(QKeyEvent *keyEvent);
	void closeEvent(QCloseEvent *closeEvent);

private:
	UserSettings *user_settings;
	GLuint tex, frame_tex;
	void hide_border();

	bool run_flag;
	bool loaded;
	float last_time;
	//int last_sec;
	QTimer *timer;
	void apply_gb_settings();
	void load_boot_roms();
	QElapsedTimer *elapsed;
	AudioBuffer *audio_buf;
	void init_audio();
	void timer_tick();
	void update_joystick();
	int gb_button(int key);
	FILE *audio_file;

	char overlay_msg[1024];
	int overlay_time;
};

#endif