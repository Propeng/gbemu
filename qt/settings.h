#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtGui/qkeysequence.h>
extern "C" {
	#include "emu/gb.h"
}

#define KEYBIND_UP 0
#define KEYBIND_DOWN 1
#define KEYBIND_LEFT 2
#define KEYBIND_RIGHT 3
#define KEYBIND_START 4
#define KEYBIND_SELECT 5
#define KEYBIND_A 6
#define KEYBIND_B 7

typedef struct {
	int bindings[8];
	GBType cgb_hw;
	GBType sgb_hw;
	GBType dmg_hw;
	int enable_sound;
	int pause_unfocus;
	int emulate_lcd;
	uint32_t dmg_palette[4];
	int skip_bootrom;
} UserSettings;

#endif