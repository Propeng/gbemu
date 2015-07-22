#ifndef PRINTERPREVIEW_H
#define PRINTERPREVIEW_H

#include <QtWidgets/qdialog.h>

class PrinterPreview : public QDialog
{
public:
	PrinterPreview(QImage **image, bool disableContinue = false);

private:
	QImage **image;
	void save();
	void discard();
};

#endif