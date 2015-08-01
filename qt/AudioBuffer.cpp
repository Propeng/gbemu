#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <QtCore/qqueue.h>
#include <SFML/Audio/SoundStream.hpp>
#include "AudioBuffer.h"

AudioBuffer::AudioBuffer(int buf_size, int channels, int sample_rate) : sf::SoundStream() {
	this->buf_size = buf_size;
	buf_ptr = 0;
	buf_queue = new QQueue<char*>();

	this->channels = channels;
	this->sample_rate = sample_rate;
	initialize(channels, sample_rate);
}

AudioBuffer::~AudioBuffer()  {
	while (buf_queue->length() > 0) {
		free(buf_queue->dequeue());
	}
	buf_ptr = 0;
}

bool AudioBuffer::onGetData(Chunk &data) {
	if (buf_queue->empty()) {
		data.samples = (sf::Int16*)malloc(buf_size);
		memset((void*)data.samples, 0, buf_size);
		data.sampleCount = buf_size / 2;
		return true;
	}

	int len = buf_size-buf_ptr;
	char *buf = buf_queue->head();

	data.samples = (sf::Int16*)malloc(len);
	memcpy((void*)data.samples, buf+buf_ptr, len);
	data.sampleCount = len / 2;

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

	return true;
}

int AudioBuffer::write(const void *data, int maxSize) {
	int len = maxSize > buf_size ? buf_size : maxSize;
	char *buf = (char*)malloc(buf_size);
	memset(buf, 0, buf_size);
	memcpy(buf, data, len);
	buf_queue->enqueue(buf);
	return len;
}

void AudioBuffer::onSeek(sf::Time timeOffset) {
	return;
}