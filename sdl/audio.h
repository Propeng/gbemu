#include <SDL.h>
#undef main

#ifndef SDL_AUDIO_H
#define SDL_AUDIO_H

void init_sdl_audio(int sample_rate, int channels, int buffer_samples, SDL_AudioCallback callback, void *data);
void pause_audio();
void unpause_audio();

#endif