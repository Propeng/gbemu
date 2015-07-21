#include <SDL.h>
#undef main
#include "audio.h"

void init_sdl_audio(int sample_rate, int channels, int buffer_samples, SDL_AudioCallback callback, void *data) {
	SDL_AudioSpec audiospec = { 0 };

	SDL_Init(SDL_INIT_AUDIO);
	
	audiospec.freq = sample_rate;
	audiospec.format = AUDIO_F32SYS;
	audiospec.channels = channels;
	audiospec.samples = buffer_samples;
	audiospec.callback = callback;
	audiospec.userdata = data;
	SDL_OpenAudio(&audiospec, NULL);
}

void pause_audio() {
	SDL_PauseAudio(1);
}

void unpause_audio() {
	SDL_PauseAudio(0);
}