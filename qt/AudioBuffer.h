#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <SFML/Audio/SoundStream.hpp>

class AudioBuffer : public sf::SoundStream
{
public:
	AudioBuffer(int buf_size, int channels, int sample_rate);
	~AudioBuffer();
	int write(const void *data, int maxSize);
	bool onGetData(Chunk &data);
	void onSeek(sf::Time timeOffset);

private:
	int channels, sample_rate, buf_size, buf_ptr;
	QQueue<char*> *buf_queue;
};

#endif