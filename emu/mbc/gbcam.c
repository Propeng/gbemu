#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "mbc/mbc.h"

uint8_t* gbcam_mem(GBContext *gb, uint16_t addr, int direction) {
	GBCamContext *mbc = (GBCamContext*)gb->mbc_context;

	if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (mbc->ram_bank & 0x10) { // camera i/o
			return mbc->camio+(addr-0xA000);
		} else {
			if (!mbc->enable_ram) return 0;
			return mbc->ram+(addr-0xA000)+(0x2000*(mbc->ram_bank & 0xF));
		}
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

uint8_t gbcam_filter(GBContext *gb, uint16_t addr, uint8_t oldval, uint8_t val, int direction) {
	GBCamContext *mbc = (GBCamContext*)gb->mbc_context;

	if (direction == MEM_READ) {
		if (addr == 0xA000 && (mbc->ram_bank & 0x10)) {
			return 0; //
		} else if (addr > 0xA000 && addr <= 0xA035 && (mbc->ram_bank & 0x10)) {
			return 0;
		} else if (addr >= 0xA100 && addr <= 0xAEFF && !(mbc->ram_bank & 0x10)) {
			return 0xCC;
		}
	}

	return val;
}

void gbcam_dump(GBContext *gb, GBBuffer *buf) {
	GBCamContext *mbc = (GBCamContext*)gb->mbc_context;
	buf->data_len = sizeof(mbc->ram);
	buf->data = mbc->ram;
}

void gbcam_load(GBContext *gb, GBBuffer *buf) {
	GBCamContext *mbc = (GBCamContext*)gb->mbc_context;
	memcpy(mbc->ram, buf->data, buf->data_len <= sizeof(mbc->ram) ? buf->data_len : sizeof(mbc->ram));
}

void gbcam_init(GBContext *gb) {
	GBCamContext *mbc = (GBCamContext*)malloc(sizeof(GBCamContext));
	memset(mbc, 0, sizeof(GBCamContext));
	gb->mbc_context = mbc;
	gb->mbc_mem = gbcam_mem;
	gb->mbc_dump = gbcam_dump;
	gb->mbc_load = gbcam_load;
	gb->mbc_filter = gbcam_filter;
	mbc->rom_bank0 = 1;
}