#ifndef KEYENTRY_H
#define KEYENTRY_H

#include <QtWidgets/qdialog.h>

class KeyEntry : public QDialog
{
public:
	KeyEntry(const char *key_name);
	int pressed_key, pressed_key_type, pressed_device;

protected:
	void keyPressEvent(QKeyEvent *keyEvent);

private:
	void poll_joysticks();
};

#endif