#include "gb.h"

#ifndef AUDIO_H
#define AUDIO_H

double channel_frequency(GBContext *gb, int channel);
double channel_duty(GBContext *gb, int channel);
int sound_length(GBContext *gb, int channel);
double volume_envelope(GBContext *gb, int channel);
float get_sample(GBContext *gb, int n, GBSoundChannel *ch);
void sound_tick(GBContext *gb, int cycles);

#endif