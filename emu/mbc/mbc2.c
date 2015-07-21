#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "mbc/mbc.h"

int mbc2_rom_bank(GBContext *gb) {
	MBC2Context *mbc = (MBC2Context*)gb->mbc_context;
	if (mbc->rom_bank == 0) {
		return 1;
	} else {
		return mbc->rom_bank & 0x0F;
	}
}

uint8_t* mbc2_mem(GBContext *gb, uint16_t addr, int direction) {
	MBC2Context *mbc = (MBC2Context*)gb->mbc_context;

	if (addr >= 0xA000 && addr <= 0xA1FF) {
		if (!mbc->enable_ram) return 0;
		return mbc->ram+(addr-0xA000);
	} else if (direction == MEM_READ) {
		if (addr <= 0x3FFF) {
			return gb->rom+addr;
		} else if (addr >= 0x4000 && addr <= 0x7FFF) {
			return gb->rom+(addr-0x4000)+(0x4000*mbc2_rom_bank(gb));
		} else {
			return 0;
		}
	} else if (direction == MEM_WRITE) {
		if (addr <= 0x1FFF) {
			return &mbc->enable_ram;
		} else if (addr >= 0x2000 && addr <= 0x3FFF && addr & 0x100) {
			return &mbc->rom_bank;
		} else {
			return 0;
		}
	}
	return 0;
}

void mbc2_dump(GBContext *gb, GBBuffer *buf) {
	MBC2Context *mbc = (MBC2Context*)gb->mbc_context;
	buf->data_len = sizeof(mbc->ram);
	buf->data = mbc->ram;
}

void mbc2_load(GBContext *gb, GBBuffer *buf) {
	MBC2Context *mbc = (MBC2Context*)gb->mbc_context;
	memcpy(mbc->ram, buf->data, buf->data_len <= sizeof(mbc->ram) ? buf->data_len : sizeof(mbc->ram));
}

void mbc2_init(GBContext *gb) {
	MBC2Context *mbc = (MBC2Context*)malloc(sizeof(MBC2Context));
	memset(mbc, 0, sizeof(MBC2Context));
	gb->mbc_context = mbc;
	gb->mbc_mem = mbc2_mem;
	gb->mbc_dump = mbc2_dump;
	gb->mbc_load = mbc2_load;
}