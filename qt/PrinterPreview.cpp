#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qfiledialog.h>
#include <QtGui/qpixmap.h>
#include "PrinterPreview.h"

PrinterPreview::PrinterPreview(QImage **image, bool disableContinue) : QDialog() {
	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSizeConstraint(QLayout::SetFixedSize);
	
	QLabel *label = new QLabel();
	label->setAlignment(Qt::AlignCenter);
	label->setPixmap(QPixmap::fromImage(**image));
	vbox->addWidget(label);

	vbox->addWidget(new QLabel("What do you want to do with this printout?"));

	QPushButton *continueBtn = new QPushButton("Continue printing");
	connect(continueBtn, &QPushButton::pressed, this, &PrinterPreview::close);
	continueBtn->setEnabled(!disableContinue);
	vbox->addWidget(continueBtn);

	QPushButton *saveBtn = new QPushButton("Save and reset printer buffer");
	connect(saveBtn, &QPushButton::pressed, this, &PrinterPreview::save);
	vbox->addWidget(saveBtn);

	QPushButton *discardBtn = new QPushButton("Discard buffer without saving");
	connect(discardBtn, &QPushButton::pressed, this, &PrinterPreview::discard);
	vbox->addWidget(discardBtn);

	setLayout(vbox);
	setModal(true);
	setWindowTitle("Printer Buffer");
	this->image = image;
}

void PrinterPreview::save() {
	QString filename = QFileDialog::getSaveFileName(this, "Save Printout", "", "PNG files (*.png);;All files (*.*)");
	if (filename.length() > 0) {
		(*image)->save(filename);
		delete *image;
		*image = NULL;
		close();
	}
}

void PrinterPreview::discard() {
	delete *image;
	*image = NULL;
	close();
}
