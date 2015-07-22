#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <QtCore/qiodevice.h>
#include <QtCore/qqueue.h>
#include "AudioBuffer.h"

AudioBuffer::AudioBuffer(int buf_size) : QIODevice() {
	this->buf_size = buf_size;
	buf_ptr = 0;
	buf_queue = new QQueue<char*>();
}

AudioBuffer::~AudioBuffer()  {
	close();
}

qint64 AudioBuffer::readData(char *data, qint64 maxSize) {
	if (buf_queue->length() < 1) {
		printf("Audio buffer underrun!\n");
		return 0;
	}
	int len = maxSize > buf_size-buf_ptr ? buf_size-buf_ptr : maxSize;
	char *buf = buf_queue->head();
	memcpy(data, buf+buf_ptr, len);

	buf_ptr += len;
	if (buf_ptr >= buf_size) {
		buf_queue->dequeue();
		if ((buf_queue->length() > 1)) {
			printf("Too much latency, skipping...\n");
			buf_ptr = 0;
			while (buf_queue->length() > 0) {
				buf_queue->dequeue();
			}
		}
		free(buf);
		buf_ptr = 0;
	}
	
	return len;
}

qint64 AudioBuffer::writeData(const char *data, qint64 maxSize) {
	int len = maxSize > buf_size ? buf_size : maxSize;
	char *buf = (char*)malloc(buf_size);
	memset(buf, 0, buf_size);
	memcpy(buf, data, len);
	buf_queue->enqueue(buf);
	return len;
}

bool AudioBuffer::atEnd() const {
	return buf_queue->length() == 0;
}

qint64 AudioBuffer::bytesAvailable() const {
	return buf_queue->length() * buf_size - buf_ptr;
}

qint64 AudioBuffer::bytesToWrite() const {
	return 0;
}

bool AudioBuffer::canReadLine() const {
	return false;
}

void AudioBuffer::close() {
	while (buf_queue->length() > 0) {
		free(buf_queue->dequeue());
	}
	buf_ptr = 0;
}

bool AudioBuffer::isSequential() const {
	return false;
}

bool AudioBuffer::open(OpenMode mode) {
	QIODevice::open(mode);
	return true;
}

qint64 AudioBuffer::pos() const {
	return 0;
}

bool AudioBuffer::reset() {
	return false;
}

bool AudioBuffer::seek(qint64 pos) {
	return false;
}

qint64 AudioBuffer::size() const {
	return bytesAvailable();
}

bool AudioBuffer::waitForBytesWritten(int msecs) {
	return false;
}

bool AudioBuffer::waitForReadyRead(int msecs) {
	return false;
}

qint64 AudioBuffer::readLineData(char *data, qint64 maxSize) {
	return 0;
}

bool AudioBuffer::isOpen() const {
	return true;
}