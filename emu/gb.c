#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "gb.h"
#include "cpu/cpu.h"
#include "mbc/mbc.h"
#include "video/video.h"
#include "sound/sound.h"

GBContext* init_context() {
	GBContext *gb = (GBContext*)malloc(sizeof(GBContext));
	memset(gb, 0, sizeof(GBContext));
	srand(0);
	gb->key_states = 0xFF;
	gb->settings.save_interval = 3;
	gb->settings.sample_rate = 44100;
	return gb;
}

void destroy_context(GBContext *gb) {
	free(gb->mbc_context);
	free(gb->rom);
	free(gb->last_framebuf);
	free(gb);
}

void key_state(GBContext *gb, int key, int state) {
	int oldstate = gb->key_states;
	set_mask(gb->key_states, key, state ? 0 : 0xFF);
	if (gb->key_states != oldstate) {
		req_interrupt(gb, INT_JOYPAD);
	}
}

void key_mask(GBContext *gb, uint8_t mask) {
	int oldstate = gb->key_states;
	gb->key_states = mask;
	if (gb->key_states != oldstate) {
		req_interrupt(gb, INT_JOYPAD);
	}
}

int get_keys_mask(GBContext *gb) {
	if ((gb->io[IO_JOYP] & MASK_JOYP_SELDIR) == 0) {
		return gb->key_states & 0xF;
	} else if ((gb->io[IO_JOYP] & MASK_JOYP_SELBTN) == 0) {
		return (gb->key_states & 0xF0) >> 4;
	}
	return 0x0F;
}

uint32_t* run_frame(GBContext *gb) {
	int period;
	int multiplier = gb->double_speed ? 2 : 1;
	int last_cycles;
	int clk = gb->cycles;
	uint32_t *framebuf;
	int reading_vblank = 0;

	if (gb->settings.paused) {
		framebuf = gb->last_framebuf;
	} else {
		if (gb->sgb.vblank_read) {
			reading_vblank = 1;
		}

		video_mode(gb, 1);
		if (gb->io[IO_LCDC] & MASK_LCDC_ENABLE)
			req_interrupt(gb, INT_VBLANK);
		for (period = 0; period < 10; period++) {
			update_ly(gb, DISPLAY_HEIGHT+period);
			run_cycles(gb, (CLK_MODE1/10)*multiplier);
		}

		if (gb->last_framebuf == NULL) {
			gb->last_framebuf = (uint32_t*)malloc(DISPLAY_WIDTH*DISPLAY_HEIGHT*sizeof(uint32_t));
		}
		framebuf = gb->last_framebuf;
		for (gb->io[IO_LCD_LY] = 0; gb->io[IO_LCD_LY] < DISPLAY_HEIGHT; gb->io[IO_LCD_LY]++) {
			compare_ly(gb, gb->io[IO_LCD_LY]);
			video_mode(gb, 2);
			run_cycles(gb, CLK_MODE2*multiplier);
			if (!gb->skip_video) draw_scanline(gb, framebuf, gb->io[IO_LCD_LY]);
			video_mode(gb, 3);
			run_cycles(gb, CLK_MODE3*multiplier);
			video_mode(gb, 0);
			run_cycles(gb, CLK_MODE0*multiplier);
		}
		update_ly(gb, DISPLAY_HEIGHT);
		
		//hack for some roms where ly is checked exactly at vblank
		last_cycles = gb->cycles;
		cpu_cycle(gb);
		gb->extra_cycles += gb->cycles-last_cycles;

		if (reading_vblank) {
			gb->sgb.vblank_read = 0;
			gb->sgb.vblank_callback(gb);
		}
	}

	gb->frame_counter++;
	if (gb->frame_counter % FRAMES_PER_SEC == 0 && gb->mbc_timer != NULL)
		gb->mbc_timer(gb);

	if (gb->has_battery && !gb->settings.paused && gb->frame_counter % (FRAMES_PER_SEC*gb->settings.save_interval) == 0 && gb->settings.save_ram != NULL)
		gb->settings.save_ram(gb->settings.callback_data);

	return framebuf;
}

void skip_bootrom(GBContext *gb) {
	int nframes = 0;
	void *snd_callback = gb->settings.play_sound;
	gb->settings.play_sound = NULL;
	run_frame(gb);
	gb->skip_video = 1;
	while (gb->io[IO_BOOTROM] == 0) {
		run_frame(gb);
		nframes++;
		if (nframes >= FRAMES_PER_SEC*10) break; //if boot rom is still running after 10 seconds then it will probably never exit
	}
	gb->skip_video = 0;
	run_frame(gb);
	gb->settings.play_sound = snd_callback;
}