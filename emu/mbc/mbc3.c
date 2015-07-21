#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "mbc/mbc.h"

int mbc3_rom_bank(GBContext *gb) {
	MBC3Context *mbc = (MBC3Context*)gb->mbc_context;
	if (mbc->rom_bank == 0) {
		return 1;
	} else {
		return mbc->rom_bank & 0x7F;
	}
}

uint8_t* mbc3_mem(GBContext *gb, uint16_t addr, int direction) {
	MBC3Context *mbc = (MBC3Context*)gb->mbc_context;

	if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (!mbc->enable_ram) return 0;
		if (mbc->ram_bank <= 3) {
			return mbc->ram+(addr-0xA000)+(0x2000*mbc->ram_bank);
		} else if (mbc->ram_bank >= 8 && mbc->ram_bank <= 0xC) {
			return mbc->rtc+(mbc->ram_bank-8);
		} else {
			return 0;
		}
	} else if (direction == MEM_READ) {
		if (addr <= 0x3FFF) {
			return gb->rom+addr;
		} else if (addr >= 0x4000 && addr <= 0x7FFF) {
			return gb->rom+(addr-0x4000)+(0x4000*mbc3_rom_bank(gb));
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
			return &mbc->latch;
		} else {
			return 0;
		}
	}
	return 0;
}

uint8_t mbc3_filter(GBContext *gb, uint16_t addr, uint8_t oldval, uint8_t val, int direction) {
	MBC3Context *mbc = (MBC3Context*)gb->mbc_context;

	if (direction == MEM_WRITE) {
		//if (addr <= 0x1FFF) { //enable ram
		//	if (val == 0x00 && oldval == 0x0A) gb->needs_to_save = 1;
		if (addr >= 0x6000 && addr <= 0x7FFF) { //latch
			if (oldval == 0 && val == 1) {
				mbc->is_latched = !mbc->is_latched;
			}
		}
	}
	return val;
}

void mbc3_timer(GBContext *gb) {
	MBC3Context *mbc = (MBC3Context*)gb->mbc_context;

	if (!mbc->is_latched && !(mbc->rtc[MBC3_RTC_FLAGS] & MASK_RTC_HALT)) {
		mbc->rtc[MBC3_RTC_SECONDS]++;
		while (mbc->rtc[MBC3_RTC_SECONDS] >= 60) {
			mbc->rtc[MBC3_RTC_SECONDS] -= 60;
			mbc->rtc[MBC3_RTC_MINUTES]++;
			while (mbc->rtc[MBC3_RTC_MINUTES] >= 60) {
				mbc->rtc[MBC3_RTC_MINUTES] -= 60;
				mbc->rtc[MBC3_RTC_HOURS]++;
				while (mbc->rtc[MBC3_RTC_HOURS] >= 24) {
					mbc->rtc[MBC3_RTC_HOURS] -= 24;
					mbc->rtc[MBC3_RTC_DAYLOW]++;
					if (mbc->rtc[MBC3_RTC_DAYLOW] == 0) {
						if (mbc->rtc[MBC3_RTC_FLAGS] & MASK_RTC_DAYHIGH) {
							set_mask(mbc->rtc[MBC3_RTC_FLAGS], MASK_RTC_OVERFLOW, 0xFF);
						} else {
							set_mask(mbc->rtc[MBC3_RTC_FLAGS], MASK_RTC_DAYHIGH, 0xFF);
						}
					}
				}
			}
		}
		//printf("%d:%d:%d\n", mbc->rtc[MBC3_RTC_HOURS], mbc->rtc[MBC3_RTC_MINUTES], mbc->rtc[MBC3_RTC_SECONDS]);
	}
}

void mbc3_dump(GBContext *gb, GBBuffer *buf) {
	MBC3Context *mbc = (MBC3Context*)gb->mbc_context;
	buf->data_len = sizeof(mbc->ram);
	buf->data = mbc->ram;
	buf->rtc_len = sizeof(mbc->rtc);
	buf->rtc_data = mbc->rtc;
}

void mbc3_load(GBContext *gb, GBBuffer *buf) {
	MBC3Context *mbc = (MBC3Context*)gb->mbc_context;
	if (buf->rtc_len == sizeof(mbc->rtc)) {
		memcpy(mbc->ram, buf->data, sizeof(mbc->ram));
		memcpy(mbc->rtc, buf->rtc_data, sizeof(mbc->rtc));
	} else {
		//memcpy(mbc->rtc, buf->data, buf->data_len <= sizeof(mbc->rtc) ? buf->data_len : sizeof(mbc->rtc));
		//memcpy(mbc->ram, buf->data+sizeof(mbc->rtc), (buf->data_len-sizeof(mbc->rtc)) <= sizeof(mbc->ram) ? (buf->data_len-sizeof(mbc->ram)) : sizeof(mbc->ram));
		memcpy(mbc->ram, buf->data, sizeof(mbc->ram));
		memcpy(mbc->rtc, buf->data+sizeof(mbc->ram), sizeof(mbc->rtc));
	}
}

void mbc3_init(GBContext *gb) {
	MBC3Context *mbc = (MBC3Context*)malloc(sizeof(MBC3Context));
	memset(mbc, 0, sizeof(MBC3Context));
	gb->mbc_context = mbc;
	gb->mbc_mem = mbc3_mem;
	gb->mbc_filter = mbc3_filter;
	gb->mbc_timer = mbc3_timer;
	gb->mbc_dump = mbc3_dump;
	gb->mbc_load = mbc3_load;
}