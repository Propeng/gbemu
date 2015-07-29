#include <string.h>
#include <stdio.h>
#include "gb.h"
#include "sgb.h"

void sgb_attr_blk(GBContext *gb, uint8_t *data) {
	int fill_out, fill_in, fill_border;
	int pal_out, pal_in, pal_border;
	int x1, x2, y1, y2;
	int x, y;

	fill_in = data[0] & 1;
	fill_border = data[0] & (1<<1);
	fill_out = data[0] & (1<<2);

	pal_in = data[1] & 3;
	pal_border = (data[1] >> 2) & 3;
	pal_out = (data[1] >> 4) & 3;
	if (fill_out && !fill_in && !fill_border) {
		fill_border = 1;
		pal_border = pal_out;
	}
	if (!fill_out && fill_in && !fill_border) {
		fill_border = 1;
		pal_border = pal_in;
	}

	x1 = data[2]; y1 = data[3]; x2 = data[4]; y2 = data[5]; 

	for (y = 0; y < SGB_YCHARS; y++) {
		for (x = 0; x < SGB_XCHARS; x++) {
			if (x > x1 && x < x2 && y > y1 && y < y2) {
				if (fill_in) gb->sgb.palette_map[y][x] = pal_in;
			} else if (((x == x1 || x == x2) && y >= y1 && y <= y2) || ((y == y1 || y == y2) && x >= x1 && x <= x2)) {
				if (fill_border) gb->sgb.palette_map[y][x] = pal_border;
			} else {
				if (fill_out) gb->sgb.palette_map[y][x] = pal_out;
			}
		}
	}
}

void sgb_attr_lin(GBContext *gb, uint8_t *data) {
	int line = data[0] & 0x1F;
	int pal = (data[0] >> 5) & 3;
	int horiz = data[0] >> 7;
	int n;

	if (horiz) {
		for (n = 0; n < SGB_XCHARS; n++) {
			gb->sgb.palette_map[line][n] = pal;
		}
	} else {
		for (n = 0; n < SGB_YCHARS; n++) {
			gb->sgb.palette_map[n][line] = pal;
		}
	}
}

void sgb_attr_div(GBContext *gb, uint8_t *data) {
	int pal2 = data[0] & 3;
	int pal1 = (data[0] >> 2) & 3;
	int paldiv = (data[0] >> 4) & 3;
	int horiz = (data[0] >> 6) & 1;
	int line = data[1];
	int n, m;

	if (horiz) {
		for (m = 0; m < line; m++) {
			for (n = 0; n < SGB_XCHARS; n++) {
				gb->sgb.palette_map[m][n] = pal1;
			}
		}
		for (n = 0; n < SGB_XCHARS; n++) {
			gb->sgb.palette_map[line][n] = paldiv;
		}
		for (m = line+1; m < SGB_YCHARS; m++) {
			for (n = 0; n < SGB_XCHARS; n++) {
				gb->sgb.palette_map[m][n] = pal2;
			}
		}
	} else {
		for (m = 0; m < line; m++) {
			for (n = 0; n < SGB_YCHARS; n++) {
				gb->sgb.palette_map[n][m] = pal1;
			}
		}
		for (n = 0; n < SGB_YCHARS; n++) {
			gb->sgb.palette_map[n][line] = paldiv;
		}
		for (m = line+1; m < SGB_XCHARS; m++) {
			for (n = 0; n < SGB_YCHARS; n++) {
				gb->sgb.palette_map[n][m] = pal2;
			}
		}
	}
}

void sgb_attr_chr(GBContext *gb, uint8_t *data, int length, int startx, int starty, int dir) {
	int n, entry, pal;
	int x = startx, y = starty;

	for (n = 0; n < length; n++) {
		entry = data[n/4];
		pal = (entry >> (6-(n%4)*2)) & 3;
		gb->sgb.palette_map[y][x] = pal;

		if (dir) { //vertical
			y++;
			if (y == SGB_YCHARS) {
				y = starty;
				x++;
			}
		} else { //horizontal
			x++;
			if (x == SGB_XCHARS) {
				x = startx;
				y++;
			}
		}
	}
}

void sgb_pal(GBContext *gb, uint16_t *data, int pal1, int pal2) {
	int i, r, g, b;
	for (i = 0; i < 7; i++) {
		r = data[i] & 0x1F;
		g = (data[i] >> 5) & 0x1F;
		b = (data[i] >> 10) & 0x1F;
		if (i < 4) { 
			gb->sgb.palettes[pal1][i][0] = (float)r / 0x1F;
			gb->sgb.palettes[pal1][i][1] = (float)g / 0x1F;
			gb->sgb.palettes[pal1][i][2] = (float)b / 0x1F;
		} else if (pal2 >= 0) {
			gb->sgb.palettes[pal2][i-3][0] = (float)r / 0x1F;
			gb->sgb.palettes[pal2][i-3][1] = (float)g / 0x1F;
			gb->sgb.palettes[pal2][i-3][2] = (float)b / 0x1F;
		}
		if (i == 0) {
			gb->sgb.backdrop[0] = (float)r / 0x1F;
			gb->sgb.backdrop[1] = (float)g / 0x1F;
			gb->sgb.backdrop[2] = (float)b / 0x1F;
		}
	}
}

void sgb_pal_set(GBContext *gb, uint16_t *data) {
	sgb_pal(gb, (uint16_t*)(gb->sgb.palette_mem+(data[0]*8)), 0, -1);
	sgb_pal(gb, (uint16_t*)(gb->sgb.palette_mem+(data[1]*8)), 1, -1);
	sgb_pal(gb, (uint16_t*)(gb->sgb.palette_mem+(data[2]*8)), 2, -1);
	sgb_pal(gb, (uint16_t*)(gb->sgb.palette_mem+(data[3]*8)), 3, -1);
}

void sgb_pal_trn_callback(GBContext *gb) {
	if (gb->io[IO_LCDC] & MASK_LCDC_SELTILEDATA) {
		memcpy(gb->sgb.palette_mem, gb->vram[0], sizeof(gb->sgb.palette_mem));
	} else {
		memcpy(gb->sgb.palette_mem, gb->vram[0]+0x800, sizeof(gb->sgb.palette_mem));
	}
}

void sgb_parse_packet(GBContext *gb) {
	int command, length, entry;

	command = gb->sgb.packet[0]/8;
	length = gb->sgb.packet[0]%8;
	
	switch (command) {
	case SGB_PAL01:
		sgb_pal(gb, (uint16_t*)(gb->sgb.packet+1), 0, 1);
		break;

	case SGB_PAL23:
		sgb_pal(gb, (uint16_t*)(gb->sgb.packet+1), 2, 3);
		break;

	case SGB_PAL03:
		sgb_pal(gb, (uint16_t*)(gb->sgb.packet+1), 0, 3);
		break;

	case SGB_PAL12:
		sgb_pal(gb, (uint16_t*)(gb->sgb.packet+1), 1, 2);
		break;

	case SGB_ATTR_BLK:
		for (entry = 0; entry < gb->sgb.packet[1]; entry++) {
			sgb_attr_blk(gb, gb->sgb.packet+2+entry*6);
		}
		break;

	case SGB_ATTR_LIN:
		for (entry = 0; entry < gb->sgb.packet[1]; entry++) {
			sgb_attr_lin(gb, gb->sgb.packet+2+entry);
		}
		break;

	case SGB_ATTR_DIV:
		sgb_attr_div(gb, gb->sgb.packet+1);
		break;

	case SGB_ATTR_CHR:
		sgb_attr_chr(gb, gb->sgb.packet+6, *(uint16_t*)(gb->sgb.packet+3), gb->sgb.packet[1], gb->sgb.packet[2], gb->sgb.packet[5]);
		break;

	case SGB_PAL_SET:
		sgb_pal_set(gb, (uint16_t*)(gb->sgb.packet+1));
		break;

	case SGB_PAL_TRN:
		gb->sgb.vblank_read = 1;
		gb->sgb.vblank_callback = sgb_pal_trn_callback;
		break;

	case SGB_MLT_REQ:
		if (gb->sgb.packet[1] == 1)
			gb->sgb.n_joypads = 2;
		else if (gb->sgb.packet[1] == 3)
			gb->sgb.n_joypads = 4;
		else
			gb->sgb.n_joypads = 1;
		gb->sgb.sel_joypad = 0;
		break;

	//case SGB_ATTR_TRN: //TODO
	//	break;

	//case SGB_ATTR_SET: //TODO
	//	break;

	case SGB_MASK_EN:
		gb->sgb.mask = gb->sgb.packet[1];
		break;

	default:
		printf("Unsupported SGB command %02X, length %u\n", command, length);
	}
}

void sgb_packet_start(GBContext *gb) {
	//memset(gb->sgb.packet, 0, sizeof(gb->sgb.packet));
	gb->sgb.packet_bit = 0;
	gb->sgb.packet_byte = 0;
	gb->sgb.joyp_redirect = 1;
}

void sgb_packet_bit(GBContext *gb, int val) {
	int length = gb->sgb.packet[0]%8;
	if (gb->sgb.packet_bit == 0 && gb->sgb.packet_byte == SGB_PACKET_LEN) { // stop bit
		//printf("command %02x length %02x\n", gb->sgb.packet[0]/8, gb->sgb.packet[0]%8);
		if (gb->sgb.packet_length <= 1) {
			if (length <= 1) {
				gb->sgb.packet_length = 1;
				sgb_parse_packet(gb);
				memset(gb->sgb.packet, 0, sizeof(gb->sgb.packet));
			} else {
				gb->sgb.packet_num = 1;
				gb->sgb.packet_length = length;
			}
		} else {
			gb->sgb.packet_num++;
			if (gb->sgb.packet_num == gb->sgb.packet_length) {
				sgb_parse_packet(gb);
				memset(gb->sgb.packet, 0, sizeof(gb->sgb.packet));
				gb->sgb.packet_num = 0;
				gb->sgb.packet_length = 0;
			}
		}

		gb->sgb.joyp_redirect = 0;
		gb->sgb.packet_bit = 0;
		gb->sgb.packet_byte = 0;
		//memset(gb->sgb.packet, 0, sizeof(gb->sgb.packet));
	} else {
		//printf("byte %u bit %u = %c\n", gb->sgb.packet_byte, gb->sgb.packet_bit, val ? 'h' : 'l');
		if (val) gb->sgb.packet[gb->sgb.packet_num*SGB_PACKET_LEN + gb->sgb.packet_byte] |= 1<<gb->sgb.packet_bit;

		gb->sgb.packet_bit++;
		if (gb->sgb.packet_bit == 8) {
			gb->sgb.packet_bit = 0;
			gb->sgb.packet_byte++;
		}
	}
}

void sgb_color_scanline(GBContext *gb, uint32_t *line, int scanline) {
	int x, xchar, ychar = scanline/8;
	float *values = NULL;
	float r = 0, g = 0, b = 0;
	int redb, greenb, blueb;
	for (xchar = 0; xchar < SGB_XCHARS; xchar++) {
		for (x = xchar*8; x < xchar*8+8; x++) {
			if (gb->sgb.mask == 3)
				values = gb->sgb.palettes[gb->sgb.palette_map[ychar][xchar]][0];
			else if (line[x] == gb->settings.dmg_palette[0])
				//values = gb->sgb.palettes[gb->sgb.palette_map[ychar][xchar]][0];
				values = gb->sgb.backdrop;
			else if (line[x] == gb->settings.dmg_palette[1])
				values = gb->sgb.palettes[gb->sgb.palette_map[ychar][xchar]][1];
			else if (line[x] == gb->settings.dmg_palette[2])
				values = gb->sgb.palettes[gb->sgb.palette_map[ychar][xchar]][2];
			else if (line[x] == gb->settings.dmg_palette[3])
				values = gb->sgb.palettes[gb->sgb.palette_map[ychar][xchar]][3];
			
			if (values != NULL) {
				r = values[0];
				g = values[1];
				b = values[2];
			}
			
			redb = (uint8_t)(r*0xFF);
			greenb = (uint8_t)(g*0xFF);
			blueb = (uint8_t)(b*0xFF);
			line[x] = blueb | (greenb << 8) | (redb << 16);
		}
	}
}

void sgb_init(GBContext *gb) {
	gb->sgb.n_joypads = 1;
	gb->sgb.joyp_mode = 1;
	
	gb->sgb.backdrop[0] = 1;
	gb->sgb.backdrop[1] = 1;
	gb->sgb.backdrop[2] = 1;
	gb->sgb.palettes[0][0][0] = 1;
	gb->sgb.palettes[0][0][1] = 1;
	gb->sgb.palettes[0][0][2] = 1;
	gb->sgb.palettes[0][1][0] = 0.66f;
	gb->sgb.palettes[0][1][1] = 0.66f;
	gb->sgb.palettes[0][1][2] = 0.66f;
	gb->sgb.palettes[0][2][0] = 0.33f;
	gb->sgb.palettes[0][2][1] = 0.33f;
	gb->sgb.palettes[0][2][2] = 0.33f;
	gb->sgb.palettes[0][3][0] = 0;
	gb->sgb.palettes[0][3][1] = 0;
	gb->sgb.palettes[0][3][2] = 0;
}