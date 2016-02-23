#include <QtWidgets/qdialog.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qtabwidget.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qgroupbox.h>
#include <QtWidgets/qradiobutton.h>
#include <QtWidgets/qbuttongroup.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qsizepolicy.h>
#include <QtWidgets/qcolordialog.h>
#include "SettingsWindow.h"
#include "KeyEntry.h"
extern "C" {
	#include "emu/gb.h"
}

SettingsWindow::SettingsWindow(UserSettings *user_settings) : QDialog() {
	this->user_settings = user_settings;

	QVBoxLayout *generalLayout = new QVBoxLayout();
	
	QGroupBox *hwTypeGroup = new QGroupBox("Hardware Type");
	QGridLayout *hwTypeLayout = new QGridLayout();
	hwTypeLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	
	hwTypeLayout->addWidget(new QLabel("Run Gameboy Color games on:"), 0, 0);
	cgbcgbRadio = new QRadioButton("CGB");
	cgbcgbRadio->setChecked(user_settings->cgb_hw == GB_CGB);
	connect(cgbcgbRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(cgbcgbRadio, 0, 1);
	cgbsgbRadio = new QRadioButton("SGB");
	cgbsgbRadio->setChecked(user_settings->cgb_hw == GB_SGB);
	connect(cgbsgbRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(cgbsgbRadio, 0, 2);
	cgbdmgRadio = new QRadioButton("DMG");
	cgbdmgRadio->setChecked(user_settings->cgb_hw == GB_DMG);
	connect(cgbdmgRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(cgbdmgRadio, 0, 3);
	
	hwTypeLayout->addWidget(new QLabel("Run Super Gameboy games on:"), 1, 0);
	sgbcgbRadio = new QRadioButton("CGB");
	sgbcgbRadio->setChecked(user_settings->sgb_hw == GB_CGB);
	connect(sgbcgbRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(sgbcgbRadio, 1, 1);
	sgbsgbRadio = new QRadioButton("SGB");
	sgbsgbRadio->setChecked(user_settings->sgb_hw == GB_SGB);
	connect(sgbsgbRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(sgbsgbRadio, 1, 2);
	sgbdmgRadio = new QRadioButton("DMG");
	sgbdmgRadio->setChecked(user_settings->sgb_hw == GB_DMG);
	connect(sgbdmgRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(sgbdmgRadio, 1, 3);
	
	hwTypeLayout->addWidget(new QLabel("Run Gameboy games on:"), 2, 0);
	dmgcgbRadio = new QRadioButton("CGB");
	dmgcgbRadio->setChecked(user_settings->dmg_hw == GB_CGB);
	connect(dmgcgbRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(dmgcgbRadio, 2, 1);
	dmgsgbRadio = new QRadioButton("SGB");
	dmgsgbRadio->setChecked(user_settings->dmg_hw == GB_SGB);
	connect(dmgsgbRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(dmgsgbRadio, 2, 2);
	dmgdmgRadio = new QRadioButton("DMG");
	dmgdmgRadio->setChecked(user_settings->dmg_hw == GB_DMG);
	connect(dmgdmgRadio, &QRadioButton::clicked, this, &SettingsWindow::update_settings);
	hwTypeLayout->addWidget(dmgdmgRadio, 2, 3);
	
	QButtonGroup *cgbBtnGroup = new QButtonGroup();
	cgbBtnGroup->addButton(cgbcgbRadio);
	cgbBtnGroup->addButton(cgbsgbRadio);
	cgbBtnGroup->addButton(cgbdmgRadio);
	QButtonGroup *sgbBtnGroup = new QButtonGroup();
	sgbBtnGroup->addButton(sgbcgbRadio);
	sgbBtnGroup->addButton(sgbsgbRadio);
	sgbBtnGroup->addButton(sgbdmgRadio);
	QButtonGroup *dmgBtnGroup = new QButtonGroup();
	dmgBtnGroup->addButton(dmgcgbRadio);
	dmgBtnGroup->addButton(dmgsgbRadio);
	dmgBtnGroup->addButton(dmgdmgRadio);

	hwTypeGroup->setLayout(hwTypeLayout);
	generalLayout->addWidget(hwTypeGroup);

	useBootChk = new QCheckBox("Use boot ROMs");
	useBootChk->setChecked(user_settings->use_bootrom);
	connect(useBootChk, &QCheckBox::clicked, this, &SettingsWindow::update_settings);
	generalLayout->addWidget(useBootChk);

	skipBootChk = new QCheckBox("Skip startup logo animation when using boot ROMs");
	skipBootChk->setChecked(user_settings->skip_bootrom);
	connect(skipBootChk, &QCheckBox::clicked, this, &SettingsWindow::update_settings);
	generalLayout->addWidget(skipBootChk);

	pauseUnfocusChk = new QCheckBox("Pause emulation while window is unfocused");
	pauseUnfocusChk->setChecked(user_settings->pause_unfocus);
	connect(pauseUnfocusChk, &QCheckBox::clicked, this, &SettingsWindow::update_settings);
	generalLayout->addWidget(pauseUnfocusChk);

	enableSoundChk = new QCheckBox("Enable sound");
	enableSoundChk->setChecked(user_settings->enable_sound);
	connect(enableSoundChk, &QCheckBox::clicked, this, &SettingsWindow::update_settings);
	generalLayout->addWidget(enableSoundChk);

	generalLayout->addStretch(1);
	QWidget *generalWidget = new QWidget();
	generalWidget->setLayout(generalLayout);

	QVBoxLayout *videoLayout = new QVBoxLayout();
	
	gbcColorsChk = new QCheckBox("Simulate GBC LCD colors (may look better or worse in some games)");
	gbcColorsChk->setChecked(user_settings->emulate_lcd);
	connect(gbcColorsChk, &QCheckBox::clicked, this, &SettingsWindow::update_settings);
	videoLayout->addWidget(gbcColorsChk);

	QGroupBox *colorsGroup = new QGroupBox("DMG Color Palette");
	QGridLayout *colorsLayout = new QGridLayout();

	defaultColorsBtn = new QPushButton("Set default grayscale colors");
	connect(defaultColorsBtn, &QPushButton::clicked, this, &SettingsWindow::set_default_colors);
	colorsLayout->addWidget(defaultColorsBtn, 0, 0, 1, 4);

	color0Btn = new QPushButton("Color 0");
	connect(color0Btn, &QPushButton::clicked, this, &SettingsWindow::color_btn_click);
	colorsLayout->addWidget(color0Btn, 1, 0);
	color0Disp = new SquareWidget();
	color0Disp->setStyleSheet(QString("background-color: #%0").arg(QString::number(user_settings->dmg_palette[0], 16)));
	colorsLayout->addWidget(color0Disp, 2, 0);

	color1Btn = new QPushButton("Color 1");
	connect(color1Btn, &QPushButton::clicked, this, &SettingsWindow::color_btn_click);
	colorsLayout->addWidget(color1Btn, 1, 1);
	color1Disp = new SquareWidget();
	color1Disp->setStyleSheet(QString("background-color: #%0").arg(QString::number(user_settings->dmg_palette[1], 16)));
	colorsLayout->addWidget(color1Disp, 2, 1);

	color2Btn = new QPushButton("Color 2");
	connect(color2Btn, &QPushButton::clicked, this, &SettingsWindow::color_btn_click);
	colorsLayout->addWidget(color2Btn, 1, 2);
	color2Disp = new SquareWidget();
	color2Disp->setStyleSheet(QString("background-color: #%0").arg(QString::number(user_settings->dmg_palette[2], 16)));
	colorsLayout->addWidget(color2Disp, 2, 2);

	color3Btn = new QPushButton("Color 3");
	connect(color3Btn, &QPushButton::clicked, this, &SettingsWindow::color_btn_click);
	colorsLayout->addWidget(color3Btn, 1, 3);
	color3Disp = new SquareWidget();
	color3Disp->setStyleSheet(QString("background-color: #%0").arg(QString::number(user_settings->dmg_palette[3], 16)));
	colorsLayout->addWidget(color3Disp, 2, 3);

	colorsGroup->setLayout(colorsLayout);
	videoLayout->addWidget(colorsGroup);

	videoLayout->addStretch(1);
	QWidget *videoWidget = new QWidget();
	videoWidget->setLayout(videoLayout);

	QGridLayout *keysLayout = new QGridLayout();
	keysLayout->setAlignment(Qt::AlignVCenter);

	QLabel *upLabel = new QLabel("Up");
	upLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(upLabel, 0, 0);
	upBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_UP]));
	connect(upBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(upBtn, 0, 1);

	QLabel *downLabel = new QLabel("Down");
	downLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(downLabel, 1, 0);
	downBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_DOWN]));
	connect(downBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(downBtn, 1, 1);

	QLabel *leftLabel = new QLabel("Left");
	leftLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(leftLabel, 2, 0);
	leftBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_LEFT]));
	connect(leftBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(leftBtn, 2, 1);

	QLabel *rightLabel = new QLabel("Right");
	rightLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(rightLabel, 3, 0);
	rightBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_RIGHT]));
	connect(rightBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(rightBtn, 3, 1);

	QLabel *startLabel = new QLabel("Start");
	startLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(startLabel, 0, 2);
	startBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_START]));
	connect(startBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(startBtn, 0, 3);

	QLabel *selectLabel = new QLabel("Select");
	selectLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(selectLabel, 1, 2);
	selectBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_SELECT]));
	connect(selectBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(selectBtn, 1, 3);

	QLabel *aLabel = new QLabel("A");
	aLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(aLabel, 2, 2);
	aBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_A]));
	connect(aBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(aBtn, 2, 3);

	QLabel *bLabel = new QLabel("B");
	bLabel->setAlignment(Qt::AlignCenter);
	keysLayout->addWidget(bLabel, 3, 2);
	bBtn = new QPushButton(key_string(user_settings->bindings[KEYBIND_B]));
	connect(bBtn, &QPushButton::clicked, this, &SettingsWindow::binding_btn_click);
	keysLayout->addWidget(bBtn, 3, 3);
	
	QWidget *keysWidget = new QWidget();
	keysWidget->setLayout(keysLayout);

	QTabWidget *tabs = new QTabWidget();
	tabs->addTab(generalWidget, "General");
	tabs->addTab(videoWidget, "Video");
	tabs->addTab(keysWidget, "Key Bindings");

	QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSizeConstraint(QLayout::SetFixedSize);
	vbox->addWidget(tabs);
	vbox->addWidget(btns);
	setLayout(vbox);
	setModal(true);
}

void SettingsWindow::update_settings() {
	if (cgbcgbRadio->isChecked()) user_settings->cgb_hw = GB_CGB;
	else if (cgbsgbRadio->isChecked()) user_settings->cgb_hw = GB_SGB;
	else if (cgbdmgRadio->isChecked()) user_settings->cgb_hw = GB_DMG;
	if (sgbcgbRadio->isChecked()) user_settings->sgb_hw = GB_CGB;
	else if (sgbsgbRadio->isChecked()) user_settings->sgb_hw = GB_SGB;
	else if (sgbdmgRadio->isChecked()) user_settings->sgb_hw = GB_DMG;
	if (dmgcgbRadio->isChecked()) user_settings->dmg_hw = GB_CGB;
	else if (dmgsgbRadio->isChecked()) user_settings->dmg_hw = GB_SGB;
	else if (dmgdmgRadio->isChecked()) user_settings->dmg_hw = GB_DMG;
	
	user_settings->use_bootrom = useBootChk->isChecked();
	user_settings->skip_bootrom = skipBootChk->isChecked();
	user_settings->pause_unfocus = pauseUnfocusChk->isChecked();
	user_settings->enable_sound = enableSoundChk->isChecked();
	user_settings->emulate_lcd = gbcColorsChk->isChecked();
}

void SettingsWindow::set_default_colors() {
	user_settings->dmg_palette[0] = 0x00F0F0F0;
	user_settings->dmg_palette[1] = 0x00C0C0C0;
	user_settings->dmg_palette[2] = 0x00808080;
	user_settings->dmg_palette[3] = 0x00404040;
	color0Disp->setStyleSheet("background-color: #F0F0F0");
	color1Disp->setStyleSheet("background-color: #C0C0C0");
	color2Disp->setStyleSheet("background-color: #808080");
	color3Disp->setStyleSheet("background-color: #404040");
}

void SettingsWindow::color_btn_click() {
	QPushButton *btn = (QPushButton*)sender();
	int color;
	if (btn == color0Btn)
		color = 0;
	else if (btn == color1Btn)
		color = 1;
	else if (btn == color2Btn)
		color = 2;
	else if (btn == color3Btn)
		color = 3;

	QColorDialog colorDialog(QColor(QString("#%0").arg(QString::number(user_settings->dmg_palette[color], 16))));
	if (colorDialog.exec() == QDialog::Accepted) {
		
		if (btn == color0Btn) {
			color0Disp->setStyleSheet(QString("background-color: %0").arg(colorDialog.selectedColor().name()));
			user_settings->dmg_palette[0] = colorDialog.selectedColor().rgb();
		} else if (btn == color1Btn) {
			color1Disp->setStyleSheet(QString("background-color: %0").arg(colorDialog.selectedColor().name()));
			user_settings->dmg_palette[1] = colorDialog.selectedColor().rgb();
		} else if (btn == color2Btn) {
			color2Disp->setStyleSheet(QString("background-color: %0").arg(colorDialog.selectedColor().name()));
			user_settings->dmg_palette[2] = colorDialog.selectedColor().rgb();
		} else if (btn == color3Btn) {
			color3Disp->setStyleSheet(QString("background-color: %0").arg(colorDialog.selectedColor().name()));
			user_settings->dmg_palette[3] = colorDialog.selectedColor().rgb();
		}
	}
}

void SettingsWindow::binding_btn_click() {
	QPushButton *btn = (QPushButton*)sender();
	char *keyname;
	if (btn == upBtn)
		keyname = "Up";
	else if (btn == downBtn)
		keyname = "Down";
	else if (btn == leftBtn)
		keyname = "Left";
	else if (btn == rightBtn)
		keyname = "Right";
	else if (btn == startBtn)
		keyname = "Start";
	else if (btn == selectBtn)
		keyname = "Select";
	else if (btn == aBtn)
		keyname = "A";
	else if (btn == bBtn)
		keyname = "B";

	KeyEntry entry(keyname);
	if (entry.exec() == QDialog::Accepted) {
		KeyBinding newkey;
		newkey.device = entry.pressed_device;
		newkey.type = entry.pressed_key_type;
		newkey.key = entry.pressed_key;

		btn->setText(key_string(newkey));
		if (btn == upBtn)
			user_settings->bindings[KEYBIND_UP] = newkey;
		else if (btn == downBtn)
			user_settings->bindings[KEYBIND_DOWN] = newkey;
		else if (btn == leftBtn)
			user_settings->bindings[KEYBIND_LEFT] = newkey;
		else if (btn == rightBtn)
			user_settings->bindings[KEYBIND_RIGHT] = newkey;
		else if (btn == startBtn)
			user_settings->bindings[KEYBIND_START] = newkey;
		else if (btn == selectBtn)
			user_settings->bindings[KEYBIND_SELECT] = newkey;
		else if (btn == aBtn)
			user_settings->bindings[KEYBIND_A] = newkey;
		else if (btn == bBtn)
			user_settings->bindings[KEYBIND_B] = newkey;
	}
}

QString SettingsWindow::key_string(KeyBinding key) {
	if (key.device == -1) {
		if (key.key == Qt::Key_Control)
			return QString("Ctrl");
		if (key.key == Qt::Key_Alt)
			return QString("Alt");
		if (key.key == Qt::Key_Shift)
			return QString("Shift");
		return QKeySequence(key.key).toString();
	} else if (key.type == KEYTYPE_BTN) {
		return QString().sprintf("Button %u:%u", key.device, key.key);
	}  else if (key.type == KEYTYPE_AXISPOS) {
		return QString().sprintf("Axis %u:%u+", key.device, key.key);
	} else if (key.type == KEYTYPE_AXISNEG) {
		return QString().sprintf("Axis %u:%u-", key.device, key.key);
	}
	return QString();
}