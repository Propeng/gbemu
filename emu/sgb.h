#include <stdint.h>

#ifndef SGB_H_STRUCT
#define SGB_H_STRUCT

#define SGB_XCHARS 20
#define SGB_YCHARS 18
#define SGB_DISPLAY_WIDTH 256
#define SGB_DISPLAY_HEIGHT 224
#define SGB_FRAME_WIDTH 48
#define SGB_FRAME_HEIGHT 40
#define SGB_TILE_SIZE 8

typedef struct {
	int joyp_redirect;
	uint8_t packet[0x100];
	int packet_byte;
	int packet_bit;
	int packet_length;
	int packet_num;

	int vblank_read;
	void (*vblank_callback)(void *gb);

	int n_joypads;
	int sel_joypad;
	int joyp_mode;

	uint8_t palette_map[SGB_YCHARS][SGB_XCHARS];
	float palettes[4][4][3];
	float backdrop[3];
	int mask;
	uint8_t palette_mem[0x1000];
	uint8_t atf_mem[45][90];
	uint8_t tile_mem[0x100][0x20];
} SGBContext;

#endif

#ifndef DEF_STRUCTS
#ifndef SBG_H
#define SBG_H

#define SGB_PACKET_LEN 0x10

#define SGB_PAL01 0x00
#define SGB_PAL23 0x01
#define SGB_PAL03 0x02
#define SGB_PAL12 0x03
#define SGB_ATTR_BLK 0x04
#define SGB_ATTR_LIN 0x05
#define SGB_ATTR_DIV 0x06
#define SGB_ATTR_CHR 0x07
#define SGB_PAL_SET 0x0A
#define SGB_PAL_TRN 0x0B
#define SGB_MLT_REQ 0x11
#define SGB_CHR_TRN 0x13
#define SGB_PCT_TRN 0x14
#define SGB_ATTR_TRN 0x15
#define SGB_ATTR_SET 0x16
#define SGB_MASK_EN 0x17

static const char *SGB_CMD_NAMES[] = { "PAL01", "PAL23", "PAL03", "PAL12", "ATTR_BLK", "ATTR_LIN", "ATTR_DIV", "ATTR_CHR", "SOUND", "SOU_TRN", "PAL_SET", "PAL_TRN", "ATRC_EN",
	"TEST_EN", "ICON_EN", "DATA_SND", "DATA_TRN", "MLT_REQ", "JUMP", "CHR_TRN", "PCT_TRN", "ATTR_TRN", "ATTR_SET", "MASK_EN", "OBJ_TRN", "PAL_PRI", };

void sgb_init(GBContext *gb);
void sgb_packet_start(GBContext *gb);
void sgb_packet_bit(GBContext *gb, int val);
void sgb_color_scanline(GBContext *gb, uint32_t *line, int scanline);

#endif
#endif