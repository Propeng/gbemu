#include <stdint.h>
#include <SDL.h>
#undef main

#ifndef SDL_INPUT_H
#define SDL_INPUT_H

#define INPUT_UNMAPPED 0
#define INPUT_KEYBOARD 1
#define INPUT_JOYSTICK_BTN 2
#define INPUT_JOYSTICK_HAT 3
#define INPUT_JOYSTICK_AXIS 4

typedef int KeyType;
typedef struct {
	KeyType type;
	int device;
	int val;
	int user_state;
} InputButton;

typedef struct {
	InputButton up;
	InputButton down;
	InputButton left;
	InputButton right;
	InputButton start;
	InputButton select;
	InputButton a;
	InputButton b;
} InputMappings;

void init_sdl_input();
void clear_mappings();
InputMappings *get_mappings();
uint8_t get_gb_btnmap();
int get_btn_state(InputButton btn);

#endif