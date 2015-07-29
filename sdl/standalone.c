#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <SDL.h>
#include "emu/gb.h"
#include "emu/rom.h"
#include "emu/video/video.h"
#include "emu/cpu/cpu.h"

GBContext* gb;
float audiobuf[SND_BUFLEN];
FILE *audio_file;
int nsamples = 0;
int nreads = 0, nwrites = 0;

void load_ram() {
	FILE *save_file;
	GBBuffer buf = { 0 };
	char filename[1024];
	sprintf(filename, "%s.sav", gb->rom_title_safe);

	save_file = fopen(filename, "rb");
	if (save_file == NULL) {
		printf("-- Failed to read save file %s: %s\n", filename, strerror(errno));
		return;
	}
	
	fseek(save_file, 0, SEEK_END);
	buf.data_len = ftell(save_file);
	fseek(save_file, 0, 0);

	buf.data = (uint8_t*)malloc(buf.data_len);
	fread(buf.data, 1, buf.data_len, save_file);
	fclose(save_file);
	ram_load(gb, &buf);
	free(buf.data);
	
	printf("-- Loaded save from %s.\n", filename);
}

void save_ram(void *data) {
	FILE *save_file;
	GBBuffer buf = { 0 };
	char filename[1024];
	sprintf(filename, "%s.sav", gb->rom_title_safe);
	
	save_file = fopen(filename, "wb");
	if (save_file == NULL) {
		printf("-- Failed to open file %s for saving: %s\n", filename, strerror(errno));
		return;
	}
	fseek(save_file, 0, SEEK_SET);
	ram_dump(gb, &buf);
	fwrite(buf.data, 1, buf.data_len, save_file);
	if (buf.rtc_len > 0) fwrite(buf.rtc_data, 1, buf.rtc_len, save_file);
	fflush(save_file);
	fclose(save_file);

	printf("-- Saved to %s.\n", filename);
}

void play_sound(float *samples, int n_bytes, void *data) {
	//SDL_LockAudio();
	//memset(audiobuf, 0, sizeof(audiobuf));
	memcpy(audiobuf, samples, n_bytes);
	fwrite(samples, 1, n_bytes, audio_file);
	nwrites++;
	nsamples += n_bytes/sizeof(float)/N_CHANNELS;
	//SDL_UnlockAudio();
}

void load_gb() {
	FILE *rom = NULL;
	size_t len;
	uint8_t *buf;

	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Tetris (V1.1) (JU) [!].gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Spot.gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Nemesis.gb", "rb");
	rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Hugo.gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Adventure Island (USA, Europe).gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Asteroids & Missile Command.gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Pokemon Red.gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Pokemon - Gold Version (USA, Europe).gbc", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Pokemon - Crystal Version (USA, Europe).gbc", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Missile Command.gbc", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Skate or Die - Bad 'N Rad.gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Sylvester and Tweety - Breakfast on the Run (Europe) (En,Fr,De,Es,It,Nl).gbc", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Donkey Kong Land 2 (USA, Europe).gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Motocross Maniacs (E) [!].gb", "rb");
	//rom = fopen("F:\\Taken from New Linux\\Games\\Emulators\\VisualBoyAdvance\\gbroms\\Gameboy Camera.gb", "rb");
	//rom = fopen("C:\\Users\\Propeng\\Downloads\\cpu_instrs\\cpu_instrs\\cpu_instrs.gb", "rb");
	//rom = fopen("C:\\Users\\Propeng\\Downloads\\instr_timing\\instr_timing\\instr_timing.gb", "rb");
	if (rom == NULL) {
		printf("Failed to open ROM file\n");
		exit(1);
	}

	fseek(rom, 0, SEEK_END);
	len = ftell(rom);
	fseek(rom, 0, 0);
	buf = (uint8_t*)malloc(len);
	fread(buf, 1, len, rom);
	fclose(rom);

	gb = init_context();
	gb->settings.emulate_lcd = 1;
	gb->settings.save_ram = save_ram;
	gb->settings.play_sound = play_sound;
	load_rom(gb, buf, len);
}

void show_tilemap() {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event ev;
	int running = 1;
	uint8_t *framebuf;
	int tilex, tiley;
	SDL_Rect rect;

	window = SDL_CreateWindow("BG Tile Map", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, TILE_WIDTH*0x10*2, TILE_HEIGHT*0x10*2, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	
	while (running) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_MOUSEBUTTONDOWN:
				running = 0;
				break;
			}
		}
		
		SDL_RenderClear(renderer);
		
		for (tiley = 0; tiley < 0x10; tiley++) {
			for (tilex = 0; tilex < 0x10; tilex++) {
				framebuf = tile_rgb_bitmap(gb, 0, tiley*0x10+tilex);
				texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, TILE_WIDTH, TILE_HEIGHT);
				SDL_UpdateTexture(texture, NULL, framebuf, TILE_WIDTH*3);
				rect.x = tilex*TILE_WIDTH*2;
				rect.y = tiley*TILE_HEIGHT*2;
				rect.w = TILE_WIDTH*2;
				rect.h = TILE_HEIGHT*2;
				SDL_RenderCopy(renderer, texture, NULL, &rect);
				SDL_DestroyTexture(texture);
				free(framebuf);
			}
		}

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void show_cgb_palette() {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Event ev;
	int running = 1;
	int palx, paly;
	uint32_t color;

	window = SDL_CreateWindow("CGB Palette", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 8*20, 8*20, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetLogicalSize(renderer, 8, 8);
	
	while (running) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_MOUSEBUTTONDOWN:
				running = 0;
				break;
			}
		}
		
		SDL_RenderClear(renderer);
		
		for (paly = 0; paly < 8; paly++) {
			for (palx = 0; palx < 4; palx++) {
				color = cgb_palette_color(gb, gb->cgb_bg_palette+(paly*8), palx);
				SDL_SetRenderDrawColor(renderer, color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, 255);
				SDL_RenderDrawPoint(renderer, palx, paly);
			}
		}
		for (paly = 0; paly < 8; paly++) {
			for (palx = 0; palx < 4; palx++) {
				color = cgb_palette_color(gb, gb->cgb_obj_palette+(paly*8), palx);
				SDL_SetRenderDrawColor(renderer, color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, 255);
				SDL_RenderDrawPoint(renderer, palx+4, paly);
			}
		}

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

int btn_mapping(int btn) {
	switch (btn) {
	case 1:
		return BTN_A;
	case 2:
		return BTN_B;
	case 8:
		return BTN_SELECT;
	case 9:
		return BTN_START;
	}
	return 0;
}

int hat_mapping(int hat) {
	int val = 0;
	if (hat & 1)
		val |= BTN_UP;
	if (hat & 2)
		val |= BTN_RIGHT;
	if (hat & 4)
		val |= BTN_DOWN;
	if (hat & 8)
		val |= BTN_LEFT;
	return val;
}

void audio_callback(void *data, Uint8 *stream, int len) {
	if (len != sizeof(audiobuf))
		printf("Audio buffer size mismatch!\n");
	memset(stream, 0, len);
	if (gb->settings.paused) return;
	if (nreads < nwrites) {
		memcpy(stream, audiobuf, len);
		memset(audiobuf, 0, sizeof(audiobuf));
		//SDL_MixAudio(stream, (Uint8*)audiobuf, len, 16);
		nreads++;
	} else {
		printf("Audio buffer underrun!\n");
	}
}

//int main(int argc, char *argv[]) {
int main_sdl(int argc, char *argv[]) {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event ev;
	int running = 1;
	uint32_t *framebuf;
	int frames_start = 0, ticks_start = 0;
	char title[64];
	int i, last_nsamples = 0;
	SDL_AudioSpec audiospec = { 0 };

	audio_file = fopen("samples", "wb");

	load_gb();
	if (gb->has_battery) load_ram();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
	window = SDL_CreateWindow(gb->rom_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DISPLAY_WIDTH*2, DISPLAY_HEIGHT*2, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	audiospec.freq = gb->settings.sample_rate;
	audiospec.format = AUDIO_F32SYS;
	audiospec.channels = N_CHANNELS;
	audiospec.samples = SND_BUFLEN/N_CHANNELS;
	audiospec.callback = audio_callback;
	SDL_OpenAudio(&audiospec, NULL);
	SDL_PauseAudio(0);

	SDL_JoystickEventState(SDL_ENABLE);
	SDL_JoystickOpen(1);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	while (running) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_MOUSEBUTTONDOWN:
				if (ev.button.button == 3) {
					if (gb->settings.paused) {
						gb->settings.paused = 0;
						printf("-- Emulation resumed.\n");
					} else {
						gb->settings.paused = 1;
						printf("-- Emulation paused.\n");
					}
				}
				break;

			case SDL_KEYDOWN:
				if (ev.key.keysym.scancode == SDL_SCANCODE_1) {
					show_tilemap();
				} else if (ev.key.keysym.scancode == SDL_SCANCODE_2) {
					show_cgb_palette();
				}
				break;

			case SDL_JOYBUTTONDOWN:
				if (btn_mapping(ev.jbutton.button)) {
					key_state(gb, btn_mapping(ev.jbutton.button), BTN_PRESSED);
				}
				break;

			case SDL_JOYBUTTONUP:
				if (btn_mapping(ev.jbutton.button)) {
					key_state(gb, btn_mapping(ev.jbutton.button), BTN_UNPRESSED);
				}
				break;

			case SDL_JOYHATMOTION:
				key_state(gb, BTN_UP|BTN_DOWN|BTN_LEFT|BTN_RIGHT, BTN_UNPRESSED);
				if (hat_mapping(ev.jhat.value)) {
					key_state(gb, hat_mapping(ev.jhat.value), BTN_PRESSED);
				}
				break;

			//case SDL_JOYAXISMOTION:
			//	printf("%d %d\n", ev.jaxis.axis, ev.jaxis.value);
			//	break;

			case SDL_QUIT:
				if (gb->has_battery) save_ram(NULL);
				exit(0);
				running = 0;
				break;
			}
		}

		if (SDL_GetTicks()-ticks_start >= 1000) {
			sprintf(title, "%s - %u fps (%d%%)", gb->rom_title, gb->frame_counter-frames_start, (int)((float)(gb->frame_counter-frames_start) / (float)FRAMES_PER_SEC * 100));
			SDL_SetWindowTitle(window, title);
			if (!gb->settings.paused) printf("%u frames, %u samples/sec\n", gb->frame_counter-frames_start, nsamples-last_nsamples);
			last_nsamples = nsamples;
			ticks_start = SDL_GetTicks();
			frames_start = gb->frame_counter;
		}

		for (i = 0; i < (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_SPACE] ? 10 : 1); i++) {
			framebuf = run_frame(gb);
		}
		SDL_UpdateTexture(texture, NULL, framebuf, DISPLAY_WIDTH*sizeof(uint32_t));

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	return 0;
}