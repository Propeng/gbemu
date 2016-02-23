#include <stdint.h>
#include "gb.h"

#ifndef MBC_H
#define MBC_H

#define MBC3_RTC_SECONDS 0
#define MBC3_RTC_MINUTES 1
#define MBC3_RTC_HOURS 2
#define MBC3_RTC_DAYLOW 3
#define MBC3_RTC_FLAGS 4

#define MASK_RTC_DAYHIGH (1<<0)
#define MASK_RTC_HALT (1<<6)
#define MASK_RTC_OVERFLOW (1<<7)

#define LARGEST_MBC_SIZE sizeof(MBC5Context)

typedef struct {
	uint8_t enable_ram;
	uint8_t rom_bank;
	uint8_t ram_bank;
	uint8_t mode;
	uint8_t ram[BANK_SIZE_8KB*4];
} MBC1Context;

typedef struct {
	uint8_t enable_ram;
	uint8_t rom_bank;
	uint8_t ram[0x200];
} MBC2Context;

typedef struct {
	uint8_t enable_ram;
	uint8_t rom_bank;
	uint8_t ram_bank;
	uint8_t latch;
	int is_latched;
	uint8_t rtc[5];
	uint8_t ram[BANK_SIZE_8KB*4];
} MBC3Context;

typedef struct {
	uint8_t enable_ram;
	uint8_t rom_bank0;
	uint8_t rom_bank1;
	uint8_t ram_bank;
	uint8_t ram[BANK_SIZE_8KB*0x10];
} MBC5Context;

typedef struct {
	uint8_t enable_ram;
	uint8_t rom_bank0;
	uint8_t rom_bank1;
	uint8_t ram_bank;
	uint8_t ram[BANK_SIZE_8KB*0x10];
	uint8_t camio[BANK_SIZE_8KB];
} GBCamContext;

int mbc_init(GBContext *gb);
size_t mbc_size(GBContext *gb);

int mbc1_rom_bank(GBContext *gb);
int mbc1_ram_bank(GBContext *gb);
uint8_t* mbc1_mem(GBContext *gb, uint16_t addr, int direction);
void mbc1_dump(GBContext *gb, GBBuffer *buf);
void mbc1_load(GBContext *gb, GBBuffer *buf);
void mbc1_init(GBContext *gb);

int mbc2_ram_bank(GBContext *gb);
uint8_t* mbc2_mem(GBContext *gb, uint16_t addr, int direction);
void mbc2_dump(GBContext *gb, GBBuffer *buf);
void mbc2_load(GBContext *gb, GBBuffer *buf);
void mbc2_init(GBContext *gb);

int mbc3_ram_bank(GBContext *gb);
uint8_t* mbc3_mem(GBContext *gb, uint16_t addr, int direction);
uint8_t mbc3_filter(GBContext *gb, uint16_t addr, uint8_t oldval, uint8_t val, int direction);
void mbc3_timer(GBContext *gb);
void mbc3_dump(GBContext *gb, GBBuffer *buf);
void mbc3_load(GBContext *gb, GBBuffer *buf);
void mbc3_init(GBContext *gb);

uint8_t* mbc5_mem(GBContext *gb, uint16_t addr, int direction);
void mbc5_dump(GBContext *gb, GBBuffer *buf);
void mbc5_load(GBContext *gb, GBBuffer *buf);
void mbc5_init(GBContext *gb);

uint8_t* gbcam_mem(GBContext *gb, uint16_t addr, int direction);
uint8_t gbcam_filter(GBContext *gb, uint16_t addr, uint8_t oldval, uint8_t val, int direction);
void gbcam_dump(GBContext *gb, GBBuffer *buf);
void gbcam_load(GBContext *gb, GBBuffer *buf);
void gbcam_init(GBContext *gb);

#endif