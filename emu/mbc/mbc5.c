#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "mbc/mbc.h"

uint8_t* mbc5_mem(GBContext *gb, uint16_t addr, int direction) {
	MBC5Context *mbc = (MBC5Context*)gb->mbc_context;

	if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (!mbc->enable_ram) return 0;
		return mbc->ram+(addr-0xA000)+(0x2000*(mbc->ram_bank & 0xF));
	} else if (direction == MEM_READ) {
		if (addr <= 0x3FFF) {
			return gb->rom+addr;
		} else if (addr >= 0x4000 && addr <= 0x7FFF) {
			return gb->rom+(addr-0x4000)+(0x4000*(mbc->rom_bank0 | ((mbc->rom_bank1 & 1) << 8)));
		} else {
			return 0;
		}
	} else if (direction == MEM_WRITE) {
		if (addr <= 0x1FFF) {
			return &mbc->enable_ram;
		} else if (addr >= 0x2000 && addr <= 0x2FFF) {
			return &mbc->rom_bank0;
		} else if (addr >= 0x3000 && addr <= 0x3FFF) {
			return &mbc->rom_bank1;
		} else if (addr >= 0x4000 && addr <= 0x5FFF) {
			return &mbc->ram_bank;
		} else {
			return 0;
		}
	}
	return 0;
}

void mbc5_dump(GBContext *gb, GBBuffer *buf) {
	MBC5Context *mbc = (MBC5Context*)gb->mbc_context;
	buf->data_len = sizeof(mbc->ram);
	buf->data = mbc->ram;
}

void mbc5_load(GBContext *gb, GBBuffer *buf) {
	MBC5Context *mbc = (MBC5Context*)gb->mbc_context;
	memcpy(mbc->ram, buf->data, buf->data_len <= sizeof(mbc->ram) ? buf->data_len : sizeof(mbc->ram));
}

void mbc5_init(GBContext *gb) {
	MBC5Context *mbc = (MBC5Context*)malloc(sizeof(MBC5Context));
	memset(mbc, 0, sizeof(MBC5Context));
	gb->mbc_context = mbc;
	gb->mbc_mem = mbc5_mem;
	gb->mbc_dump = mbc5_dump;
	gb->mbc_load = mbc5_load;
}