#include <QtWidgets/qdialog.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtGui/qevent.h>
#include "KeyEntry.h"

KeyEntry::KeyEntry(const char *key_name) : QDialog() {
	pressed_key = 0;
	QVBoxLayout *layout = new QVBoxLayout();
	QLabel *label = new QLabel(QString("Press a key to bind to button ") + key_name + "...");
	label->setMargin(15);
	layout->addWidget(label);
	setLayout(layout);
	setWindowTitle("Bind Key");
}

void KeyEntry::keyPressEvent(QKeyEvent *keyEvent) {
	pressed_key = keyEvent->key();
	accept();
}