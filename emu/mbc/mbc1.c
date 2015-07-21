#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "mbc/mbc.h"

int mbc1_rom_bank(GBContext *gb) {
	MBC1Context *mbc = (MBC1Context*)gb->mbc_context;
	int bank;
	if (mbc->mode & 1) {
		bank = mbc->rom_bank & 0x1F;
	} else {
		bank = (mbc->rom_bank & 0x1F) | ((mbc->ram_bank & 3) << 5);
	}
	switch (bank) {
	case 0x00:
		return 0x01;
	case 0x20:
		return 0x21;
	case 0x40:
		return 0x41;
	case 0x60:
		return 0x61;
	default:
		return bank;
	}
}

int mbc1_ram_bank(GBContext *gb) {
	MBC1Context *mbc = (MBC1Context*)gb->mbc_context;
	if (mbc->mode & 1) {
		return mbc->ram_bank & 3;
	} else {
		return 0;
	}
}

uint8_t* mbc1_mem(GBContext *gb, uint16_t addr, int direction) {
	MBC1Context *mbc = (MBC1Context*)gb->mbc_context;

	if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (!mbc->enable_ram) return 0;
		return mbc->ram+(addr-0xA000)+(0x2000*mbc1_ram_bank(gb));
	} else if (direction == MEM_READ) {
		if (addr <= 0x3FFF) {
			return gb->rom+addr;
		} else if (addr >= 0x4000 && addr <= 0x7FFF) {
			return gb->rom+(addr-0x4000)+(0x4000*mbc1_rom_bank(gb));
		} else {
			return 0;
		}
	} else if (direction == MEM_WRITE) {
		if (addr <= 0x1FFF) {
			return &mbc->enable_ram;
		} else if (addr >= 0x2000 && addr <= 0x3FFF) {
			return &mbc->rom_bank;
		} else if (addr >= 0x4000 && addr <= 0x5FFF) {
			return &mbc->ram_bank;
		} else if (addr >= 0x6000 && addr <= 0x7FFF) {
			return &mbc->mode;
		} else {
			return 0;
		}
	}
	return 0;
}

void mbc1_dump(GBContext *gb, GBBuffer *buf) {
	MBC1Context *mbc = (MBC1Context*)gb->mbc_context;
	buf->data_len = sizeof(mbc->ram);
	buf->data = mbc->ram;
}

void mbc1_load(GBContext *gb, GBBuffer *buf) {
	MBC1Context *mbc = (MBC1Context*)gb->mbc_context;
	memcpy(mbc->ram, buf->data, buf->data_len <= sizeof(mbc->ram) ? buf->data_len : sizeof(mbc->ram));
}

void mbc1_init(GBContext *gb) {
	MBC1Context *mbc = (MBC1Context*)malloc(sizeof(MBC1Context));
	memset(mbc, 0, sizeof(MBC1Context));
	gb->mbc_context = mbc;
	gb->mbc_mem = mbc1_mem;
	gb->mbc_dump = mbc1_dump;
	gb->mbc_load = mbc1_load;
}