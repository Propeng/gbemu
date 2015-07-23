#include <string.h>
#include <stdio.h>
#include "gb.h"
#include "mem.h"
#include "video/video.h"
#include "cpu/cpu.h"

uint8_t* mem_ptr(GBContext *gb, uint16_t addr, int direction) {
	if (addr <= 0xFF && !gb->io[IO_BOOTROM] && !gb->cgb_mode) { //dmg boot rom
		return gb->dmg_bootrom+addr;
	} else if (addr <= 0xFF && !gb->io[IO_BOOTROM] && gb->cgb_mode) { //cgb boot rom low
		return gb->cgb_bootrom+addr;
	} else if (addr >= 0x200 && addr <= 0x8FF && !gb->io[IO_BOOTROM] && gb->cgb_mode) { //cgb boot rom high
		return gb->cgb_bootrom+addr;
	} else if (addr <= 0x7FFF) { //rom banks
		if (gb->mbc_enabled) return gb->mbc_mem(gb, addr, direction);
		if (direction == MEM_WRITE) return 0;
		return gb->rom+addr;
	} else if (addr >= 0x8000 && addr <= 0x9FFF) { //vram
		if (gb->cgb_mode) {
			return gb->vram[gb->io[IO_VRAMBANK]]+(addr-0x8000);
		} else {
			return gb->vram[0]+(addr-0x8000);
		}
	} else if (addr >= 0xA000 && addr <= 0xBFFF) { //external ram
		if (gb->mbc_enabled) return gb->mbc_mem(gb, addr, direction);
		return gb->eram+(addr-0xA000);
	} else if (addr >= 0xC000 && addr <= 0xCFFF) { //work ram bank 0
		return gb->wram[0]+(addr-0xC000);
	} else if (addr >= 0xD000 && addr <= 0xDFFF) { //work ram bank 1-7
		if (gb->cgb_mode) {
			return gb->io[IO_WRAMBANK] == 0 ? gb->wram[1]+(addr-0xD000) : gb->wram[gb->io[IO_WRAMBANK]]+(addr-0xD000);
		} else {
			return gb->wram[1]+(addr-0xD000);
		}
	} else if (addr >= 0xFF00 && addr <= 0xFF7F) { //io ports
		return gb->io+(addr-0xFF00);
	} else if (addr >= 0xE000 && addr <= 0xFDFF) { //echo
		return mem_ptr(gb, addr-0x2000, direction);
	} else if (addr >= 0xFE00 && addr <= 0xFE9F) { //oam
		return gb->oam+(addr-0xFE00);
	} else if (addr >= 0xFF80 && addr <= 0xFFFE) { //high ram
		return gb->hram+(addr-0xFF80);
	} else if (addr == 0xFFFF) {
		return &gb->int_enable;
	}
	printf("Invalid memory address %04Xh\n", addr);
	gb->error = 1;
	return NULL;
}

uint8_t filter_io_read(GBContext *gb, uint16_t addr, uint8_t val) {
	uint8_t ioaddr;
	if ((addr <= 0x7FFF || addr >= 0x8000 && addr <= 0x9FFF) && gb->mbc_filter != NULL)
		return gb->mbc_filter(gb, addr, 0, val, MEM_READ);
	if (addr < 0xFF00 || addr > 0xFF7F)
		return val;
	ioaddr = (uint8_t)addr;

	switch (ioaddr) {
	//case IO_LCDSTAT:
	//	return val | (1<<7);
	case IO_HDMA_ATTR:
		if (gb->hdma_active)
			return gb->hdma_remaining & MASK_HDMA_LENGTH;
		return val;
	case IO_JOYP:
		return ((val | MASK_JOYP_UNUSED) & 0xF0) | (get_keys_mask(gb) & 0x0F);
	case IO_BGPD:
		if (!gb->cgb_mode) return val;
		return gb->cgb_bg_palette[gb->io[IO_BGPI] & MASK_PI_INDEX];
	case IO_OBPD:
		if (!gb->cgb_mode) return val;
		return gb->cgb_obj_palette[gb->io[IO_OBPI] & MASK_PI_INDEX];
	case IO_SND_ENABLE:
		return gb->channels[0].playing | (gb->channels[1].playing << 1) | (gb->channels[2].playing << 2) | (gb->channels[3].playing << 3) | (val & (1<<7)) | 0x70;
	//case IO_SERIAL_DATA:
 	//	return peripheral_read(&gb->peripheral);
	default:
		return val;
	}
}

uint8_t filter_io_write(GBContext *gb, uint16_t addr, uint8_t oldval, uint8_t val) {
	uint8_t ioaddr;
	if ((addr <= 0x7FFF || (addr >= 0x8000 && addr <= 0x9FFF)) && gb->mbc_filter != NULL)
		return gb->mbc_filter(gb, addr, oldval, val, MEM_WRITE);
	if (addr < 0xFF00 || addr > 0xFF7F)
		return val;
	ioaddr = (uint8_t)addr;

	switch (ioaddr) {
	case IO_BOOTROM: // switch into cgb's dmg simulation mode
		if (val && !(gb->rom[0x0143] == 0x80 || gb->rom[0x0143])) gb->cgb_mode = 2;
		return val;
	case IO_JOYP:
		return val & 0xF0;
	case IO_DIV:
		gb->div_counter = 0;
		gb->tim_counter = 0;
		gb->timer = 0;
		return 0;
	case IO_TAC:
		//if (!(oldval & MASK_TAC_STOP) && (val & MASK_TAC_STOP)) gb->timer = 0;
		//gb->tim_counter = 0;
		return val;
	case IO_LCDC:
		//if (!(val & MASK_LCDC_ENABLE)) {
		//	compare_ly(gb, 0);
		//	video_mode(gb, 0);
		//}
		return val;
	case IO_LCDSTAT:
		return (oldval & 0x07) | (val & 0xF8) | (1<<7);
	case IO_LCD_LY:
		//printf("Implement resetting LY %02X->%02X\n", oldval, val);
		compare_ly(gb, val);
		return val;
	//case IO_LCD_LYC:
	//	compare_ly(gb, val);
	//	return val;
	case IO_DMA_OAM:
		oam_dma(gb, val);
		return 0;
	case IO_HDMA_ATTR:
		if (val & MASK_HDMA_MODE) {
			gb->hdma_active = 1;
			gb->hdma_copied = 0;
			gb->hdma_remaining = ((val & MASK_HDMA_LENGTH)+1)*0x10;
			return val & MASK_HDMA_LENGTH;
		} else {
			if (gb->hdma_active) {
				gb->hdma_active = 0;
				return MASK_HDMA_MODE | (gb->hdma_remaining & MASK_HDMA_LENGTH);
			} else {
				hdma_transfer(gb, val);
				return 0xFF;
			}
		}
	case IO_BGPD:
		if (!gb->cgb_mode) return val;
		gb->cgb_bg_palette[gb->io[IO_BGPI] & MASK_PI_INDEX] = val;
		if (gb->io[IO_BGPI] & MASK_PI_INC) gb->io[IO_BGPI] = ((gb->io[IO_BGPI]&MASK_PI_INDEX)+1) | MASK_PI_INC;
		return val;
	case IO_OBPD:
		if (!gb->cgb_mode) return val;
		gb->cgb_obj_palette[gb->io[IO_OBPI] & MASK_PI_INDEX] = val;
		if (gb->io[IO_OBPI] & MASK_PI_INC) gb->io[IO_OBPI] = ((gb->io[IO_OBPI]&MASK_PI_INDEX)+1) | MASK_PI_INC;
		return val;
	case IO_SND1_SWEEP:
		gb->channels[0].sweep_shift = val & 7;
		gb->channels[0].sweep_direction = (val >> 3) & 1;
		gb->channels[0].sweep_time = (val >> 4) & 7;
		return val;
	case IO_SND1_FREQHIGH:
		if (val & (1<<7)) {
			gb->channels[0].playing = 1;
			gb->channels[0].restart = 1;
			//gb->channels[0].freq = 0;
			//gb->channels[0].counter = 0;
			gb->channels[0].vol = gb->io[IO_SND1_VOL] >> 4;
			gb->channels[0].vol_envolope = gb->io[IO_SND1_VOL] & 7;
			gb->channels[0].sweep_delta = 0;
		}
		return val;
	case IO_SND2_FREQHIGH:
		if (val & (1<<7)) {
			gb->channels[1].playing = 1;
			gb->channels[1].restart = 1;
			//gb->channels[1].freq = 0;
			//gb->channels[1].counter = 0;
			gb->channels[1].vol = gb->io[IO_SND2_VOL] >> 4;
			gb->channels[1].vol_envolope = gb->io[IO_SND2_VOL] & 7;
		}
		return val;
	case IO_SND3_FREQHIGH:
		if (val & (1<<7)) {
			gb->channels[2].playing = 1;
			gb->channels[2].restart = 1;
		}
		return val;
	case IO_SND4_CTR:
		if (val & (1<<7)) {
			gb->channels[3].playing = 1;
			gb->channels[3].restart = 1;
			gb->channels[3].vol = gb->io[IO_SND4_VOL] >> 4;
			gb->channels[3].vol_envolope = gb->io[IO_SND4_VOL] & 7;
		}
		return val;
	case IO_SND1_VOL:
		gb->channels[0].vol = val >> 4;
		gb->channels[0].vol_direction = (val & (1<<3)) != 0;
		gb->channels[0].vol_envolope = val & 7;
		return val;
	case IO_SND2_VOL:
		gb->channels[1].vol = val >> 4;
		gb->channels[1].vol_direction = (val & (1<<3)) != 0;
		gb->channels[1].vol_envolope = val & 7;
		return val;
	case IO_SND3_VOL:
		gb->channels[2].vol = ((val >> 5) & 3);
		if (gb->channels[2].vol == 1) gb->channels[2].vol = 0xF;
		else if (gb->channels[2].vol == 2) gb->channels[2].vol = 0x8;
		else if (gb->channels[2].vol == 3) gb->channels[2].vol = 0x4;
		return val;
	case IO_SND4_VOL:
		gb->channels[3].vol = val >> 4;
		gb->channels[3].vol_direction = (val & (1<<3)) != 0;
		gb->channels[3].vol_envolope = val & 7;
		return val;
	case IO_SERIAL_DATA:
		//printf("serial: %02x\n", val);
		//peripheral_write(&gb->peripheral, val);
		return val;
	case IO_SERIAL_CTL:
		/*if (val & MASK_SERIAL_START) {
			gb->io[IO_SERIAL_DATA] = peripheral_transfer(&gb->peripheral, gb->io[IO_SERIAL_DATA]);
			req_interrupt(gb, INT_SERIAL);
			return val & ~MASK_SERIAL_START;
		}*/
		return val;
	default:
		return val;
	}
}

uint8_t read_mem(GBContext *gb, uint16_t addr) {
	uint8_t *ptr = mem_ptr(gb, addr, MEM_READ);
	if (ptr != NULL) {
		return filter_io_read(gb, addr, *ptr);
	} else {
		return 0;
	}
}

uint16_t read_mem16(GBContext *gb, uint16_t addr) {
	return read_mem(gb, addr) | (read_mem(gb, addr+1) << 8);
}

void set_mem(GBContext *gb, uint16_t addr, uint8_t val) {
	uint8_t *ptr;
	ptr = mem_ptr(gb, addr, MEM_WRITE);
	if (ptr != NULL) {
		*ptr = filter_io_write(gb, addr, *ptr, val);
	}
}

void set_mem16(GBContext *gb, uint16_t addr, uint16_t val) {
	set_mem(gb, addr, val & 0xFF);
	set_mem(gb, addr+1, val >> 8);
}

void ram_dump(GBContext *gb, GBBuffer *buf) {
	if (!gb->has_battery) return;
	if (gb->mbc_enabled) {
		if (gb->mbc_dump == NULL) return;
		gb->mbc_dump(gb, buf);
	} else {
		buf->data_len = sizeof(gb->eram);
		buf->data = gb->eram;
	}
}

void ram_load(GBContext *gb, GBBuffer *buf) {
	if (!gb->has_battery) return;
	if (gb->mbc_enabled) {
		if (gb->mbc_load == NULL) return;
		gb->mbc_load(gb, buf);
	} else {
		memcpy(gb->eram, buf->data, buf->data_len <= sizeof(gb->eram) ? buf->data_len : sizeof(gb->eram));
	}
}