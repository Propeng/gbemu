#include <stdio.h>
#include "gb.h"
#include "mbc/mbc.h"

int mbc_init(GBContext *gb) {
	switch (gb->cartridge_type) {
	case 0x00: case 0x08: case 0x09:
		printf("No MBC required.\n");
		return 1;

	case 0x01: case 0x02: case 0x03:
		printf("Controller type: MBC1\n");
		mbc1_init(gb);
		gb->mbc_enabled = 1;
		return 1;

	case 0x05: case 0x06:
		printf("Controller type: MBC2\n");
		mbc2_init(gb);
		gb->mbc_enabled = 1;
		return 1;

	case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
		printf("Controller type: MBC3\n");
		mbc3_init(gb);
		gb->mbc_enabled = 1;
		return 1;

	case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
		printf("Controller type: MBC5\n");
		mbc5_init(gb);
		gb->mbc_enabled = 1;
		return 1;
	}
	return 0;
}

size_t mbc_size(GBContext *gb) {
	switch (gb->cartridge_type) {
	case 0x01: case 0x02: case 0x03:
		return sizeof(MBC1Context);
	case 0x05: case 0x06:
		return sizeof(MBC2Context);
	case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
		return sizeof(MBC3Context);
	case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
		return sizeof(MBC5Context);
	}
	return 0;
}