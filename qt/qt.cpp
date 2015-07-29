#include <QtWidgets/qapplication.h>
#include "GBWindow.h"

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	GBWindow win;
	win.show();

	QRect position = win.frameGeometry();
	position.moveCenter(QDesktopWidget().availableGeometry().center());
	win.move(position.topLeft());

	return app.exec();
}