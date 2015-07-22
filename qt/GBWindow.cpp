#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qapplication.h>
#include <QtGui/qevent.h>
#include <SDL.h>
#include "GBWindow.h"
#include "EmulatorWidget.h"
extern "C" {
	#include "emu/video/video.h"
}

GBWindow::GBWindow(QWidget *parent) : QMainWindow(parent) {
	SDL_CreateWindowFrom((void*)winId());

	widget = new EmulatorWidget();
	setup_menus();

	setCentralWidget(widget);
	activateWindow();
	updateGeometry();

	QRect position = widget->frameGeometry();
	position.moveCenter(QDesktopWidget().availableGeometry().center());
	move(position.topLeft());
	
	openDialog = new QFileDialog(this, "Open ROM", "", "Gameboy ROMs (*.gb *.gbc);;All files (*.*)");
	openDialog->setAcceptMode(QFileDialog::AcceptOpen);
	openDialog->setFileMode(QFileDialog::ExistingFile);
	
	screenshotDialog = new QFileDialog(this, "Save Screenshot", "", "PNG files (*.png);;All files (*.*)");
	screenshotDialog->setAcceptMode(QFileDialog::AcceptSave);
	screenshotDialog->setFileMode(QFileDialog::AnyFile);

	connect(qApp, &QApplication::applicationStateChanged, this, &GBWindow::focus_changed);
	setFocusPolicy(Qt::StrongFocus);
}

void GBWindow::setup_menus() {

	// File
	QMenu *file = menuBar()->addMenu("&File");

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

	QMenu *periph = emu->addMenu("Serial Devices");
	
	periphNone = periph->addAction("None");
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

	// Help
	QMenu *help = menuBar()->addMenu("&Help");
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