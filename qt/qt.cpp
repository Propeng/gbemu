#include <QtWidgets/qapplication.h>
#include "GBWindow.h"

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	GBWindow win;
	win.show();
	return app.exec();
}