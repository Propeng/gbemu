#include <stdint.h>
#include <SDL.h>
#include "input.h"

InputMappings mappings;
SDL_Joystick **joysticks = 0;

void init_sdl_input() {
	int i;

	SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO);
	joysticks = (SDL_Joystick**)calloc(SDL_NumJoysticks(), sizeof(void*));
	for (i = 0; i < SDL_NumJoysticks(); i++) {
		joysticks[i] = SDL_JoystickOpen(i);
	}
	memset(&mappings, 0, sizeof(InputMappings));
	//SDL_CreateWindow("CGB Palette", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 8*20, 8*20, SDL_WINDOW_SHOWN);
	
	//SDL_JoystickEventState(SDL_IGNORE);
}

void clear_mappings() {
	memset(&mappings, 0, sizeof(InputMappings));
}

InputMappings *get_mappings() {
	return &mappings;
}

uint8_t get_gb_btnmap() {
	uint8_t mask = 0;
	SDL_Event ev;
	SDL_JoystickUpdate();
	while (SDL_PollEvent(&ev)) {
		printf("%u %u\n", ev.type, ev.jbutton.button);
	}
	mask |= get_btn_state(mappings.right);
	mask |= get_btn_state(mappings.left) << 1;
	mask |= get_btn_state(mappings.up) << 2;
	mask |= get_btn_state(mappings.down) << 3;
	mask |= get_btn_state(mappings.a) << 4;
	mask |= get_btn_state(mappings.b) << 5;
	mask |= get_btn_state(mappings.select) << 6;
	mask |= get_btn_state(mappings.start) << 7;
	//printf("%d\n", SDL_JoystickGetButton(joysticks[1], 1));
	return ~mask;
}

int get_btn_state(InputButton btn) {
	switch (btn.type) {
	case INPUT_KEYBOARD:
		return btn.user_state;
	case INPUT_JOYSTICK_BTN:
		return SDL_JoystickGetButton(joysticks[btn.device], btn.val);
	}
	return 0;
}