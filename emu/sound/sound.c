#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "gb.h"
#include "sound/sound.h"
#include "cpu/cpu.h"

double channel_frequency(GBContext *gb, int channel) {
	int val;
	double r;
	GBSoundChannel ch = gb->channels[channel-1];

	switch (channel) {
	case 1:
		val = gb->io[IO_SND1_FREQLOW] | ((gb->io[IO_SND1_FREQHIGH] & 7) << 8);
		val += (int)ch.sweep_delta;
		if (val < 0 || val >= 2047) val = 0;
		return val == 0 ? 0 : 131072. / (2048 - val);
	case 2:
		val = gb->io[IO_SND2_FREQLOW] | ((gb->io[IO_SND2_FREQHIGH] & 7) << 8);
		if (val < 0 || val >= 2047) val = 0;
		return val == 0 ? 0 : 131072. / (2048 - val);
	case 3:
		val = gb->io[IO_SND3_FREQLOW] | ((gb->io[IO_SND3_FREQHIGH] & 7) << 8);
		//if (val < 0 || val > 2047) val = 0;
		return val == 0 ? 0 : 65536. / (2048 - val);
	case 4:
		val = gb->io[IO_SND4_FREQ] >> 4;
		r = gb->io[IO_SND4_FREQ] & 7;
		r = r == 0 ? 0.5 : r;
		//if (val < 0 || val > 2047) val = 0;
		return 524288. / r / powl(2, val+1);
	}
	return 0;
}

double channel_duty(GBContext *gb, int channel) {
	int val = -1;
	
	switch (channel) {
	case 1:
		val = (gb->io[IO_SND1_DUTY] >> 6);
		break;
	case 2:
		val = (gb->io[IO_SND2_DUTY] >> 6);
		break;
	}

	switch (val) {
	case 0:
		return 0.125;
	case 1:
		return 0.25;
	case 2:
		return 0.5;
	case 3:
		return 0.75;
	}
	return 0.;
}

int sound_length(GBContext *gb, int channel) {
	int val = 0;
	switch (channel) {
	case 1:
		if (!(gb->io[IO_SND1_FREQHIGH] & (1<<6))) return -1;
		val = gb->io[IO_SND1_DUTY] & 0x3F;
		return (int)((64.-val)*(1./256.)*gb->settings.sample_rate);
	case 2:
		if (!(gb->io[IO_SND2_FREQHIGH] & (1<<6))) return -1;
		val = gb->io[IO_SND2_DUTY] & 0x3F;
		return (int)((64.-val)*(1./256.)*gb->settings.sample_rate);
	case 3:
		if (!(gb->io[IO_SND3_FREQHIGH] & (1<<6))) return -1;
		val = gb->io[IO_SND3_LENGTH];
		return (int)((256.-val)*(1./256.)*gb->settings.sample_rate);
	case 4:
		if (!(gb->io[IO_SND4_CTR] & (1<<6))) return -1;
		val = gb->io[IO_SND4_LENGTH] & 0x3F;
		return (int)((64.-val)*(1./256.)*gb->settings.sample_rate);
	}
	return -1;
}

float get_sample(GBContext *gb, int n, GBSoundChannel *ch) {
	float volumeh = 0.15f, volumel = -0.15f;
	double rate = (double)gb->settings.sample_rate;
	double wavelen, mod, modf;
	float vol;
	int waven, sample;
	//double freq = channel_frequency(gb, channel);
	wavelen = rate/ch->freq;
	mod = fmodl(ch->counter, wavelen);
	if (ch->freq == 0 || wavelen == 0 || (mod >= wavelen-1 && (ch->restart || ch->freq != channel_frequency(gb, n)))) {
		ch->restart = 0;
		ch->freq = channel_frequency(gb, n);
		ch->counter = 0;
		ch->length = sound_length(gb, n);
		
		wavelen = rate/ch->freq;
		ch->duty = wavelen*channel_duty(gb, n);
		mod = 0;
	}
	if ((int)mod == 0) {
		ch->vol_playing = ch->vol;
		ch->duty = wavelen*channel_duty(gb, n);
		/*wavelen = rate/ch->freq;
		ch->duty = wavelen*channel_duty(gb, n);
		mod = fmodl(ch->counter, wavelen);*/

		if (n == 4) {
			ch->noise_rand = rand() % 2;
			ch->noise_counter++;
			if (((gb->io[IO_SND4_CTR] & (1<<3)) && ch->noise_counter >= 0x80) || ch->noise_counter >= 0x8000) {
				ch->noise_counter = 0;
				srand(0);
			}
		}
	}

	
	vol = (float)ch->vol_playing / 0xF;
	if (n <= 2) {
		//printf("%u\n", (int)(ch->freq+ch->sweep_delta));
		return mod > ch->duty ? volumel * vol : volumeh * vol;
	} else if (n == 3 && (gb->io[IO_SND3_ENABLE] & (1<<7))) {
		if (ch->freq == 0) return 0;
		modf = mod / wavelen;
		waven = (int)(modf * SND3_WAVELEN*2);
		if (waven % 2 == 0)
			sample = gb->io[IO_SND3_WAVE+(waven/2)] >> 4;
		else
			sample = gb->io[IO_SND3_WAVE+(waven/2)] & 0xF;
		if (waven+1 == SND3_WAVELEN*2) ch->restart = 1;
		return ((float)sample / 15.f * volumeh * vol) - volumeh/2;
	} else if (n == 4) {
		return ch->noise_rand ? volumeh/2 * vol : volumel/2 * vol;
	}
	return 0;
}

void sound_tick(GBContext *gb, int cycles) {
	double clocks_per_sample = (double)CLOCKS_PER_SEC / gb->settings.sample_rate;
	float sample = 0;
	int multiplier = gb->double_speed ? 2 : 1;
	int ch;
	float vol_left = (float)(gb->io[IO_SND_VOLUME] & 0x7) / 7;
	float vol_right = (float)((gb->io[IO_SND_VOLUME] >> 4) & 0x7) / 7;
	int sweep_rawfreq;

	gb->snd_cycles += cycles;
	while (gb->snd_cycles > clocks_per_sample*multiplier) {
		gb->snd_cycles -= clocks_per_sample*multiplier;
		
		gb->snd_buffer[gb->snd_ptr] = 0;
		gb->snd_buffer[gb->snd_ptr+1] = 0;
		if (gb->io[IO_SND_ENABLE] & (1<<7)) {
			for (ch = 0; ch < 4; ch++) {
				if (gb->channels[ch].playing) {
					sample = get_sample(gb, ch+1, gb->channels+ch);
					if ((gb->io[IO_SND_CHMAP] & (1<<ch)) && (gb->io[IO_SND_CHMAP] & (1<<ch<<4))) {
						gb->snd_buffer[gb->snd_ptr] += (sample * vol_left * 0.8f) * INT16_MAX;
						gb->snd_buffer[gb->snd_ptr+1] += (sample * vol_right * 0.8f) * INT16_MAX;
					} else if (gb->io[IO_SND_CHMAP] & (1<<ch)) {
						gb->snd_buffer[gb->snd_ptr+1] += (sample * vol_right) * INT16_MAX;
						gb->snd_buffer[gb->snd_ptr] += (sample * vol_right * 0.2f) * INT16_MAX;
					} else if (gb->io[IO_SND_CHMAP] & (1<<ch<<4)) {
						gb->snd_buffer[gb->snd_ptr] += (sample * vol_left) * INT16_MAX;
						gb->snd_buffer[gb->snd_ptr+1] +=( sample * vol_left * 0.2f) * INT16_MAX;
					}
				}
			}
		}

		for (ch = 0; ch < 4; ch++) {
			if (gb->channels[ch].vol_envolope > 0) {
				if (gb->snd_counter % (gb->channels[ch].vol_envolope * (gb->settings.sample_rate/64)) == 0) {
					//gb->channels[ch].vol_envolope--;
					if (gb->channels[ch].vol_direction) {
						if (gb->channels[ch].vol < 0xF) gb->channels[ch].vol++;
					} else {
						if (gb->channels[ch].vol > 0) gb->channels[ch].vol--;
					}
				}
			}
		}

		if (gb->channels[0].sweep_time > 0) {
			if (gb->snd_counter % (gb->channels[0].sweep_time * (gb->settings.sample_rate/128)) == 0) {
				sweep_rawfreq = gb->io[IO_SND1_FREQLOW] | ((gb->io[IO_SND1_FREQHIGH] & 7) << 8);
				if (gb->channels[0].sweep_direction) {
					gb->channels[0].sweep_delta -= (sweep_rawfreq+gb->channels[0].sweep_delta) / powl(2, gb->channels[0].sweep_shift);
				} else {
					gb->channels[0].sweep_delta += (sweep_rawfreq+gb->channels[0].sweep_delta) / powl(2, gb->channels[0].sweep_shift);
				}
			}
		} else {
			//gb->channels[0].sweep_delta = 0;
		}

		for (ch = 0; ch < 4; ch++) {
			if (gb->channels[ch].playing) {
				gb->channels[ch].counter++;
				if (gb->channels[ch].length >= 0 && gb->channels[ch].counter >= gb->channels[ch].length) {
					gb->channels[ch].counter = 0;
					gb->channels[ch].playing = 0;
				}
			}
		}
		gb->snd_counter++;

		gb->snd_ptr += N_CHANNELS;
		if (gb->snd_ptr >= SND_BUFLEN) {
			if (gb->settings.play_sound != NULL) gb->settings.play_sound(gb->snd_buffer, SND_BUFLEN*sizeof(int16_t), gb->settings.callback_data);
			gb->snd_ptr = 0;
		}
	}
}