#ifndef GBWINDOW_H
#define GBWINDOW_H

#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qfiledialog.h>
#include "settings.h"
#include "EmulatorWidget.h"

class GBWindow : public QMainWindow
{
public:
	GBWindow(QWidget *parent = 0);
	void toggle_pause();
	void reset();

protected:
	void closeEvent(QCloseEvent *closeEvent);
	void dragEnterEvent(QDragEnterEvent *dragEvent);
	void dropEvent(QDropEvent *dropEvent);

private:
	UserSettings user_settings;
	void save_settings();
	void load_settings();

	EmulatorWidget *widget;
	void set_window_size();
	bool paused_focus;
	QAction *saveStateAction, *loadStateAction, *pauseAction, *resetAction, *periphNone, *periphPrinter, *printerBuf;
	QAction *win1xAction, *win2xAction, *win3xAction, *win4xAction;
	QFileDialog *openDialog, *screenshotDialog;
	void setup_menus();
	void open_rom();
	void screenshot();
	void show_settings();

	void focus_changed(Qt::ApplicationState state);
	void menu_shown();
	void menu_hidden();

	void periph_disconnect();
	void periph_printer();
};

#endif