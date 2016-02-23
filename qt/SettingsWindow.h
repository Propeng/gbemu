#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qradiobutton.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qcheckbox.h>
#include "settings.h"

class SquareWidget : public QWidget
{
public:
	bool hasHeightForWidth() const { return true; }
	int heightForWidth(int w) const { return w; }
};

class SettingsWindow : public QDialog
{
public:
	SettingsWindow(UserSettings *user_settings);

private:
	QRadioButton *cgbcgbRadio, *cgbsgbRadio, *cgbdmgRadio, *sgbcgbRadio, *sgbsgbRadio, *sgbdmgRadio, *dmgcgbRadio, *dmgsgbRadio, *dmgdmgRadio;
	QPushButton *upBtn, *downBtn, *leftBtn, *rightBtn, *startBtn, *selectBtn, *aBtn, *bBtn;
	QCheckBox *useBootChk, *skipBootChk, *pauseUnfocusChk, *enableSoundChk, *gbcColorsChk;
	QPushButton *defaultColorsBtn, *color0Btn, *color1Btn, *color2Btn, *color3Btn;
	SquareWidget *color0Disp, *color1Disp, *color2Disp, *color3Disp;
	UserSettings *user_settings;
	void update_settings();
	void set_default_colors();
	void color_btn_click();
	void binding_btn_click();
	QString key_string(KeyBinding key);
};

#endif