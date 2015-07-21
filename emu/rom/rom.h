#include <stdint.h>
#include "gb.h"

#ifndef ROM_H
#define ROM_H

int get_rom_size(int val);
int load_rom(GBContext *gb, uint8_t *rom, size_t len);
void reset_gb(GBContext *gb);

#endif