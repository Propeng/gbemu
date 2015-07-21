#include <stdint.h>
#include "gb.h"
#include "video/video.h"

void oam_dma(GBContext *gb, uint8_t start) {
	uint16_t offset = 0;
	for (offset = 0; offset < 0xA0; offset++) {
		*mem_ptr(gb, 0xFE00+offset, MEM_WRITE) = *mem_ptr(gb, start*0x100+offset, MEM_READ);
	}
}

void hdma_transfer(GBContext *gb, uint8_t attr) {
	uint16_t src = ((gb->io[IO_HDMA_SRCH] << 8) | gb->io[IO_HDMA_SRCL]) & 0xFFF0;
	uint16_t dest = ((gb->io[IO_HDMA_DESTH] << 8) | gb->io[IO_HDMA_DESTL]) & 0x1FF0 | 0x8000;
	int length = ((attr & MASK_HDMA_LENGTH)+1)*0x10;
	int mode = attr & MASK_HDMA_MODE;
	int offset;
	for (offset = 0; offset < length; offset++) {
		*mem_ptr(gb, offset+dest, MEM_WRITE) = *mem_ptr(gb, offset+src, MEM_READ);
	}
}

void hdma_hblank(GBContext *gb) {
	uint16_t src = (((gb->io[IO_HDMA_SRCH] << 8) | gb->io[IO_HDMA_SRCL]) & 0xFFF0) + gb->hdma_copied;
	uint16_t dest = (((gb->io[IO_HDMA_DESTH] << 8) | gb->io[IO_HDMA_DESTL]) & 0x1FF0 | 0x8000) + gb->hdma_copied;
	int offset;
	if (!gb->hdma_active) return;

	for (offset = 0; offset < 0x10; offset++) {
		*mem_ptr(gb, dest+offset, MEM_WRITE) = *mem_ptr(gb, src+offset, MEM_READ);
	}
	
	gb->hdma_copied += 0x10;
	gb->hdma_remaining -= 0x10;
	if (gb->hdma_remaining < 0) {
		gb->hdma_active = 0;
		gb->io[IO_HDMA_ATTR] = 0xFF;
	}
}