#include "gb.h"

#ifndef MEM_H
#define MEM_H

#define IO_INTFLAG 0x0F
#define IO_BOOTROM 0x50

//joypad
#define IO_JOYP 0x00
#define MASK_JOYP_UNUSED 0xC0
#define MASK_JOYP_SELBTN (1<<5)
#define MASK_JOYP_SELDIR (1<<4)
#define MASK_JOYP_RA (1<<0)
#define MASK_JOYP_LB (1<<1)
#define MASK_JOYP_US (1<<2)
#define MASK_JOYP_DS (1<<3)

//timers
#define IO_DIV 0x04
#define IO_TIMA 0x05
#define IO_TMA 0x06

#define IO_TAC 0x07
#define MASK_TAC_SELECT 3
#define MASK_TAC_STOP (1<<2)

//lcd registers
#define IO_LCDC 0x40
#define MASK_LCDC_BGDISP (1<<0)
#define MASK_LCDC_OBJDISP (1<<1)
#define MASK_LCDC_OBJSIZE (1<<2)
#define MASK_LCDC_BGTILEMAP (1<<3)
#define MASK_LCDC_SELTILEDATA (1<<4)
#define MASK_LCDC_WINDISP (1<<5)
#define MASK_LCDC_WINTILEMAP (1<<6)
#define MASK_LCDC_ENABLE (1<<7)

#define IO_LCDSTAT 0x41
#define MASK_LCDSTAT_MODE 3
#define MASK_LCDSTAT_LYC (1<<2)
#define MASK_LCDSTAT_INT0 (1<<3)
#define MASK_LCDSTAT_INT1 (1<<4)
#define MASK_LCDSTAT_INT2 (1<<5)
#define MASK_LCDSTAT_INTLYC (1<<6)

#define IO_LCD_SCY 0x42
#define IO_LCD_SCX 0x43
#define IO_LCD_LY 0x44
#define IO_LCD_LYC 0x45
#define IO_LCD_WY 0x4A
#define IO_LCD_WX 0x4B

#define IO_LCD_BGP 0x47
#define IO_LCD_OBP0 0x48
#define IO_LCD_OBP1 0x49

#define IO_VRAMBANK 0x4F
#define IO_DMA_OAM 0x46

#define MASK_SPATTR_CGBPAL 7
#define MASK_SPATTR_BANK (1<<3)
#define MASK_SPATTR_PAL (1<<4)
#define MASK_SPATTR_XFLIP (1<<5)
#define MASK_SPATTR_YFLIP (1<<6)
#define MASK_SPATTR_PRIORITY (1<<7)

//cgb registers
#define IO_BGPI 0x68
#define IO_BGPD 0x69
#define IO_OBPI 0x6A
#define IO_OBPD 0x6B
#define MASK_PI_INDEX 0x3F
#define MASK_PI_INC (1<<7)

#define MASK_BGATTR_PAL 7
#define MASK_BGATTR_BANK (1<<3)
#define MASK_BGATTR_XFLIP (1<<5)
#define MASK_BGATTR_YFLIP (1<<6)
#define MASK_BGATTR_PRIORITY (1<<7)

#define IO_CPUSPEED 0x4D
#define IO_WRAMBANK 0x70

#define IO_HDMA_SRCH 0x51
#define IO_HDMA_SRCL 0x52
#define IO_HDMA_DESTH 0x53
#define IO_HDMA_DESTL 0x54

#define IO_HDMA_ATTR 0x55
#define MASK_HDMA_LENGTH 0x7F
#define MASK_HDMA_MODE (1<<7)

//sound registers
#define IO_SND1_SWEEP 0x10
#define IO_SND1_DUTY 0x11
#define IO_SND1_VOL 0x12
#define IO_SND1_FREQLOW 0x13
#define IO_SND1_FREQHIGH 0x14

#define IO_SND2_DUTY 0x16
#define IO_SND2_VOL 0x17
#define IO_SND2_FREQLOW 0x18
#define IO_SND2_FREQHIGH 0x19

#define IO_SND3_ENABLE 0x1A
#define IO_SND3_LENGTH 0x1B
#define IO_SND3_VOL 0x1C
#define IO_SND3_FREQLOW 0x1D
#define IO_SND3_FREQHIGH 0x1E
#define IO_SND3_WAVE 0x30
#define SND3_WAVELEN 0x10

#define IO_SND4_LENGTH 0x20
#define IO_SND4_VOL 0x21
#define IO_SND4_FREQ 0x22
#define IO_SND4_CTR 0x23

#define IO_SND_VOLUME 0x24
#define IO_SND_CHMAP 0x25
#define IO_SND_ENABLE 0x26

//serial link registers
#define IO_SERIAL_DATA 0x01
#define IO_SERIAL_CTL 0x02
#define MASK_SERIAL_START (1<<7)

#define set_mask(flag,mask,val) flag &= ~mask; flag |= val & mask;

uint8_t* mem_ptr(GBContext *gb, uint16_t addr, int direction);
uint8_t filter_io_write(GBContext *gb, uint16_t addr, uint8_t oldval, uint8_t val);
uint8_t read_mem(GBContext *gb, uint16_t addr);
uint16_t read_mem16(GBContext *gb, uint16_t addr);
void set_mem(GBContext *gb, uint16_t addr, uint8_t val);
void set_mem16(GBContext *gb, uint16_t addr, uint16_t val);

void ram_dump(GBContext *gb, GBBuffer *buf);
void ram_load(GBContext *gb, GBBuffer *buf);

#endif