#include <QtWidgets/qdialog.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtGui/qevent.h>
#include <QtCore/qtimer.h>
#include <SFML/Window/Joystick.hpp>
#include "settings.h"
#include "KeyEntry.h"

KeyEntry::KeyEntry(const char *key_name) : QDialog() {
	QVBoxLayout *layout = new QVBoxLayout();
	QLabel *label = new QLabel(QString("Press a key to bind to button ") + key_name + "...");
	label->setMargin(15);
	layout->addWidget(label);
	setLayout(layout);
	setWindowTitle("Bind Key");

	QTimer *pollTimer = new QTimer(this);
	connect(pollTimer, &QTimer::timeout, this, &KeyEntry::poll_joysticks);
	pollTimer->start(50);
}

void KeyEntry::keyPressEvent(QKeyEvent *keyEvent) {
	pressed_key = keyEvent->key();
	pressed_device = -1;
	pressed_key_type = 0;
	accept();
}

void KeyEntry::poll_joysticks() {
	sf::Joystick::update();
	for (int js = 0; js < sf::Joystick::Count; js++) {
		if (!sf::Joystick::isConnected(js)) continue;

		for (int btn = 0; btn < sf::Joystick::getButtonCount(js); btn++) {
			if (sf::Joystick::isButtonPressed(js, btn)) {
				pressed_device = js;
				pressed_key_type = KEYTYPE_BTN;
				pressed_key = btn;
				accept();
			}
		}

		for (int axis = 0; axis < sf::Joystick::AxisCount; axis++) {
			if (sf::Joystick::hasAxis(js, (sf::Joystick::Axis)axis)) {
				if (sf::Joystick::getAxisPosition(js, (sf::Joystick::Axis)axis) > 25) {
					pressed_device = js;
					pressed_key_type = KEYTYPE_AXISPOS;
					pressed_key = axis;
					accept();
				} else if (sf::Joystick::getAxisPosition(js, (sf::Joystick::Axis)axis) < -25) {
					pressed_device = js;
					pressed_key_type = KEYTYPE_AXISNEG;
					pressed_key = axis;
					accept();
				}
			}
		}
	}
}