#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <QtCore/qiodevice.h>
#include <QtCore/qqueue.h>

class AudioBuffer : public QIODevice
{
public:
	AudioBuffer(int buf_size);
	~AudioBuffer();
	bool atEnd() const;
	qint64 bytesAvailable() const;
	qint64 bytesToWrite() const;
	bool canReadLine() const;
	void close();
	bool isSequential() const;
	bool open(OpenMode mode);
	qint64 pos() const;
	bool reset();
	bool seek(qint64 pos);
	qint64 size() const;
	bool waitForBytesWritten(int msecs);
	bool waitForReadyRead(int msecs);
	qint64 readLineData(char *data, qint64 maxSize);
	bool isOpen() const;

protected:
	qint64 readData(char *data, qint64 maxSize);
	qint64 writeData(const char *data, qint64 maxSize);

private:
	int buf_size;
	int buf_ptr;
	QQueue<char*> *buf_queue;
};

#endif