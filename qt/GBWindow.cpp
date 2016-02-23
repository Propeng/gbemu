#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qapplication.h>
#include <QtGui/qevent.h>
#include <QtCore/qmimedata.h>
#include <SDL.h>
#include "GBWindow.h"
#include "EmulatorWidget.h"
#include "SettingsWindow.h"
#include "settings.h"
extern "C" {
	#include "emu/video/video.h"
}

GBWindow::GBWindow(QWidget *parent) : QMainWindow(parent) {
	load_settings();

	widget = new EmulatorWidget(this, &user_settings);
	setup_menus();

	setCentralWidget(widget);
	activateWindow();
	updateGeometry();
	
	openDialog = new QFileDialog(this, "Open ROM", "F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms", "Gameboy ROMs (*.gb *.gbc);;All files (*.*)");
	openDialog->setAcceptMode(QFileDialog::AcceptOpen);
	openDialog->setFileMode(QFileDialog::ExistingFile);
	
	screenshotDialog = new QFileDialog(this, "Save Screenshot", "", "PNG files (*.png);;All files (*.*)");
	screenshotDialog->setAcceptMode(QFileDialog::AcceptSave);
	screenshotDialog->setFileMode(QFileDialog::AnyFile);

	connect(qApp, &QApplication::applicationStateChanged, this, &GBWindow::focus_changed);
	setFocusPolicy(Qt::StrongFocus);
	setAcceptDrops(true);
}

void GBWindow::save_settings() {
	FILE *settings_file = fopen("settings.dat", "wb");
	fwrite(&user_settings, sizeof(UserSettings), 1, settings_file);
	fclose(settings_file);
}

void GBWindow::load_settings() {
	FILE *settings_file = fopen("settings.dat", "rb");
	if (settings_file != NULL) {
		fseek(settings_file, 0, SEEK_END);
		if (ftell(settings_file) != sizeof(UserSettings)) {
			fclose(settings_file);
			settings_file = NULL;
		} else {
			fseek(settings_file, 0, SEEK_SET);
			fread(&user_settings, sizeof(UserSettings), 1, settings_file);
			fclose(settings_file);
		}
	}
	if (settings_file == NULL) { //set default settings
		memset(&user_settings, 0, sizeof(UserSettings));
		for (int i = 0; i < 8; i++) {
			user_settings.bindings[i].device = -1;
			user_settings.bindings[i].type = 0;
		}
		user_settings.bindings[KEYBIND_UP].key = Qt::Key_W;
		user_settings.bindings[KEYBIND_DOWN].key = Qt::Key_S;
		user_settings.bindings[KEYBIND_LEFT].key = Qt::Key_A;
		user_settings.bindings[KEYBIND_RIGHT].key = Qt::Key_D;
		user_settings.bindings[KEYBIND_START].key = Qt::Key_Space;
		user_settings.bindings[KEYBIND_SELECT].key = Qt::Key_Control;
		user_settings.bindings[KEYBIND_A].key = Qt::Key_Slash;
		user_settings.bindings[KEYBIND_B].key = Qt::Key_Period;
		user_settings.cgb_hw = GB_CGB;
		user_settings.sgb_hw = GB_SGB;
		user_settings.dmg_hw = GB_DMG;
		user_settings.enable_sound = 1;
		user_settings.pause_unfocus = 1;
		user_settings.emulate_lcd = 1;
		user_settings.dmg_palette[0] = 0x00F0F0F0;
		user_settings.dmg_palette[1] = 0x00C0C0C0;
		user_settings.dmg_palette[2] = 0x00808080;
		user_settings.dmg_palette[3] = 0x00404040;
		user_settings.use_bootrom = 1;
		user_settings.skip_bootrom = 0;

		save_settings();
	}
}

void GBWindow::setup_menus() {
	// File
	QMenu *file = menuBar()->addMenu("&File");
	connect(file, &QMenu::aboutToShow, this, &GBWindow::menu_shown);
	connect(file, &QMenu::aboutToHide, this, &GBWindow::menu_hidden);

	connect(file->addAction("&Open ROM..."), &QAction::triggered, this, &GBWindow::open_rom);

	saveStateAction = file->addAction("&Save State");
	connect(saveStateAction, &QAction::triggered, widget, &EmulatorWidget::save_state);
	saveStateAction->setEnabled(false);
	saveStateAction->setShortcut(QKeySequence(Qt::Key_F3));
	file->addAction(saveStateAction);

	loadStateAction = file->addAction("&Load State");
	connect(loadStateAction, &QAction::triggered, widget, &EmulatorWidget::load_state);
	loadStateAction->setEnabled(false);
	loadStateAction->setShortcut(QKeySequence(Qt::Key_F4));
	file->addAction(loadStateAction);
	file->addSeparator();

	connect(file->addAction("&Take Screenshot..."), &QAction::triggered, this, &GBWindow::screenshot);
	file->addSeparator();

	connect(file->addAction("E&xit"), &QAction::triggered, this, &GBWindow::close);

	// Emulation
	QMenu *emu = menuBar()->addMenu("&Emulation");
	connect(emu, &QMenu::aboutToShow, this, &GBWindow::menu_shown);
	connect(emu, &QMenu::aboutToHide, this, &GBWindow::menu_hidden);

	pauseAction = emu->addAction("Pause");
	connect(pauseAction, &QAction::triggered, this, &GBWindow::toggle_pause);
	pauseAction->setEnabled(false);
	pauseAction->setCheckable(true);
	pauseAction->setChecked(false);
	pauseAction->setShortcut(QKeySequence(Qt::Key_F1));
	emu->addAction(pauseAction);
	
	resetAction = emu->addAction("Reset");
	connect(resetAction, &QAction::triggered, this, &GBWindow::reset);
	resetAction->setEnabled(false);
	resetAction->setShortcut(QKeySequence(Qt::Key_F2));
	emu->addAction(resetAction);
	emu->addSeparator();

	QMenu *winSize = emu->addMenu("Window Size");
	win1xAction = winSize->addAction("1x");
	connect(win1xAction, &QAction::triggered, this, &GBWindow::set_window_size);
	win2xAction = winSize->addAction("2x");
	connect(win2xAction, &QAction::triggered, this, &GBWindow::set_window_size);
	win3xAction = winSize->addAction("3x");
	connect(win3xAction, &QAction::triggered, this, &GBWindow::set_window_size);
	win4xAction = winSize->addAction("4x");
	connect(win4xAction, &QAction::triggered, this, &GBWindow::set_window_size);

	QMenu *periph = emu->addMenu("Serial Port");
	
	periphNone = periph->addAction("Disconnected");
	connect(periphNone, &QAction::triggered, this, &GBWindow::periph_disconnect);
	periphNone->setCheckable(true);
	periphNone->setChecked(true);
	
	periphPrinter = periph->addAction("Gameboy Printer");
	connect(periphPrinter, &QAction::triggered, this, &GBWindow::periph_printer);
	periphPrinter->setCheckable(true);

	periph->addSeparator();
	printerBuf = periph->addAction("Show Printer Buffer...");
	connect(printerBuf, &QAction::triggered, widget, &EmulatorWidget::show_printer_buf);
	printerBuf->setEnabled(false);

	connect(emu->addAction("Settings..."), &QAction::triggered, this, &GBWindow::show_settings);

	// Help
	QMenu *help = menuBar()->addMenu("&Help");
	connect(help, &QMenu::aboutToShow, this, &GBWindow::menu_shown);
	connect(help, &QMenu::aboutToHide, this, &GBWindow::menu_hidden);
}

void GBWindow::set_window_size() {
	int scale;
	QAction *action = (QAction*)sender();

	if (action == win1xAction)
		scale = 1;
	else if (action == win2xAction)
		scale = 2;
	else if (action == win3xAction)
		scale = 3;
	else if (action == win4xAction)
		scale = 4;
	
	if (widget->show_frame) {
		widget->resize(SGB_DISPLAY_WIDTH*scale, SGB_DISPLAY_HEIGHT*scale);
	} else {
		widget->resize(DISPLAY_WIDTH*scale, DISPLAY_HEIGHT*scale);
	}
	for (int i = 0; i < 10; i++)
		QApplication::processEvents();
	resize(widget->width(), widget->height() + menuBar()->height());
}

void GBWindow::open_rom() {
	if (openDialog->exec() == QDialog::Accepted) {
		QString filename = openDialog->selectedFiles()[0];
		if (filename.length() > 0) {
			bool running = widget->load_rom_file(filename.toLocal8Bit().constData());
			saveStateAction->setEnabled(running);
			loadStateAction->setEnabled(running);
			pauseAction->setEnabled(running);
			resetAction->setEnabled(running);
			pauseAction->setChecked(false);
			widget->gb->settings.paused = false;
			paused_focus = true;
			activateWindow();
		}
	}
}

void GBWindow::toggle_pause() {
	widget->toggle_pause();
	pauseAction->setChecked(widget->gb->settings.paused);
}

void GBWindow::reset() {
	widget->reset();
	pauseAction->setChecked(false);
}

void GBWindow::screenshot() {
	if (screenshotDialog->exec() == QDialog::Accepted) {
		QString filename = screenshotDialog->selectedFiles()[0];
		if (filename.length() > 0) {
			QImage *image;
			if (widget->gb->last_framebuf != NULL) {
				image = new QImage((uchar*)widget->gb->last_framebuf, DISPLAY_WIDTH, DISPLAY_HEIGHT, QImage::Format_RGB32);
			} else {
				image = new QImage(DISPLAY_WIDTH, DISPLAY_HEIGHT, QImage::Format_RGB32);
				image->fill(Qt::black);
			}
			image->save(filename);
		}
	}
}

void GBWindow::focus_changed(Qt::ApplicationState state) {
	if (state == Qt::ApplicationActive) {
		//if (!paused_focus) toggle_pause();
		//paused_focus = false;
		widget->window_inactive = 0;
	} else {
		//paused_focus = widget->gb->settings.paused;
		//if (!paused_focus) toggle_pause();
		widget->window_inactive = 1;
	}
}

void GBWindow::menu_shown() {
	widget->window_inactive = 1;
}

void GBWindow::menu_hidden() {
	widget->window_inactive = 0;
}

void GBWindow::show_settings() {
	int oldpause = widget->gb->settings.paused;
	widget->gb->settings.paused = 1;
	
	load_settings();
	SettingsWindow window(&user_settings);
	window.setWindowTitle(windowTitle() + " Settings");
	if (window.exec() == QDialog::Accepted) {
		save_settings();
		widget->immediate_settings();
	} else {
		load_settings();
	}

	widget->gb->settings.paused = oldpause;
	activateWindow();
}

void GBWindow::periph_disconnect() {
	periphNone->setChecked(true);
	periphPrinter->setChecked(false);
	printerBuf->setEnabled(false);
	widget->periph_disconnect();
}

void GBWindow::periph_printer() {
	periphNone->setChecked(false);
	periphPrinter->setChecked(true);
	printerBuf->setEnabled(true);
	widget->periph_printer();
}

void GBWindow::closeEvent(QCloseEvent *closeEvent) {
	widget->close();
}

void GBWindow::dragEnterEvent(QDragEnterEvent *dragEvent) {
	if (dragEvent->mimeData()->hasUrls())
		dragEvent->acceptProposedAction();
}

void GBWindow::dropEvent(QDropEvent *dropEvent) {
	bool running = widget->load_rom_file(dropEvent->mimeData()->urls()[0].toLocalFile().toUtf8().data());
	saveStateAction->setEnabled(running);
	loadStateAction->setEnabled(running);
	pauseAction->setEnabled(running);
	resetAction->setEnabled(running);
	pauseAction->setChecked(false);
	widget->gb->settings.paused = false;
	activateWindow();
	if (running)
		widget->show_msg("Loaded ROM %s.", dropEvent->mimeData()->urls()[0].toLocalFile().toUtf8().data());
}