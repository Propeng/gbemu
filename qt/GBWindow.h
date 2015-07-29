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
	bool paused_focus;
	QAction *saveStateAction;
	QAction *loadStateAction;
	QAction *pauseAction;
	QAction *resetAction;
	QAction *periphNone;
	QAction *periphPrinter;
	QAction *printerBuf;
	QFileDialog *openDialog;
	QFileDialog *screenshotDialog;
	void setup_menus();
	void open_rom();
	void screenshot();
	void focus_changed(Qt::ApplicationState state);
	void show_settings();

	void periph_disconnect();
	void periph_printer();
};

#endif