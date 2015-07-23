#include <stdlib.h>
#include <math.h>
#include "gb.h"
#include "video/video.h"
#include "cpu/cpu.h"

void update_mode(GBContext *gb, int mode) {
	switch (mode) {
	case 0:
		if (gb->hdma_active)
			hdma_hblank(gb);
		set_mask(gb->io[IO_LCDSTAT], MASK_LCDSTAT_MODE, 0);
		if (gb->io[IO_LCDSTAT] & MASK_LCDSTAT_INT0)
			req_interrupt(gb, INT_LCDSTAT);
		break;

	case 1:
		set_mask(gb->io[IO_LCDSTAT], MASK_LCDSTAT_MODE, 1);
		if (gb->io[IO_LCDSTAT] & MASK_LCDSTAT_INT1)
			req_interrupt(gb, INT_LCDSTAT);
		break;

	case 2:
		set_mask(gb->io[IO_LCDSTAT], MASK_LCDSTAT_MODE, 2);
		if (gb->io[IO_LCDSTAT] & MASK_LCDSTAT_INT2)
			req_interrupt(gb, INT_LCDSTAT);
		break;

	case 3:
		set_mask(gb->io[IO_LCDSTAT], MASK_LCDSTAT_MODE, 3);
		break;
	}
}

void video_mode(GBContext *gb, int mode) {
	if (gb->io[IO_LCDC] & MASK_LCDC_ENABLE) {
		update_mode(gb, mode);
	} else {
		//update_mode(gb, 1);
		set_mask(gb->io[IO_LCDSTAT], MASK_LCDSTAT_MODE, 0);
	}
}

void compare_ly(GBContext *gb, int ly) {
	if (ly == gb->io[IO_LCD_LYC]) {
		set_mask(gb->io[IO_LCDSTAT], MASK_LCDSTAT_INTLYC, 0xFF);
		if ((gb->io[IO_LCDSTAT] & MASK_LCDSTAT_INTLYC) && (gb->io[IO_LCDC] & MASK_LCDC_ENABLE)) {
			req_interrupt(gb, INT_LCDSTAT);
		}
	} else {
		set_mask(gb->io[IO_LCDSTAT], MASK_LCDSTAT_INTLYC, 0);
	}
}

void update_ly(GBContext *gb, int scanline) {
	gb->io[IO_LCD_LY] = scanline;
	compare_ly(gb, scanline);
}

uint8_t* tile_rgb_bitmap(GBContext *gb, int bank, int index) {
	uint8_t *addr = gb->vram[bank]+(index*0x10);
	uint8_t b0, b1;
	uint8_t *bitmap, *pixel;
	int x, y, palette_index;

	bitmap = (uint8_t*)malloc(8*8*3);
	
	for (y = 0; y < 8; y++) {
		b0 = addr[y*2];
		b1 = addr[y*2+1];
		if (index == 0) {
			free(0);
		}
		for (x = 0; x < 8; x++) {
			pixel = bitmap+(y*8+x)*3;
			palette_index = ((b0 & (0x80>>x)) != 0) | (((b1 & (0x80>>x)) != 0) << 1);
			switch (palette_index) {
			case 0:
				pixel[0] = 0xF0;
				pixel[1] = 0xF0;
				pixel[2] = 0xF0;
				break;
			case 1:
				pixel[0] = 0xC0;
				pixel[1] = 0xC0;
				pixel[2] = 0xC0;
				break;
			case 2:
				pixel[0] = 0x80;
				pixel[1] = 0x80;
				pixel[2] = 0x80;
				break;
			case 3:
				pixel[0] = 0x40;
				pixel[1] = 0x40;
				pixel[2] = 0x40;
				break;
			}
		}
	}

	return bitmap;
}

uint8_t* get_tile(GBContext *gb, uint8_t index, int bank) {
	if (gb->io[IO_LCDC] & MASK_LCDC_SELTILEDATA) {
		return gb->vram[bank]+(index*TILE_SIZE);
	} else {
		return gb->vram[bank]+0x0800+((uint8_t)(index+0x80)*TILE_SIZE);
	}
}

uint8_t* get_bg_tilemap(GBContext *gb) {
	if (gb->io[IO_LCDC] & MASK_LCDC_BGTILEMAP) {
		return gb->vram[0]+0x1C00;
	} else {
		return gb->vram[0]+0x1800;
	}
}

uint8_t* get_win_tilemap(GBContext *gb) {
	if (gb->io[IO_LCDC] & MASK_LCDC_WINTILEMAP) {
		return gb->vram[0]+0x1C00;
	} else {
		return gb->vram[0]+0x1800;
	}
}

uint8_t* get_bg_tileattr(GBContext *gb) {
	if (gb->io[IO_LCDC] & MASK_LCDC_BGTILEMAP) {
		return gb->vram[1]+0x1C00;
	} else {
		return gb->vram[1]+0x1800;
	}
}

uint8_t* get_win_tileattr(GBContext *gb) {
	if (gb->io[IO_LCDC] & MASK_LCDC_WINTILEMAP) {
		return gb->vram[1]+0x1C00;
	} else {
		return gb->vram[1]+0x1800;
	}
}

uint32_t dmg_palette_index(GBContext *gb, uint8_t palette, int palette_index) {
	int color = 0;
	switch (palette_index) {
	case 0:
		color = palette & 3;
		break;
	case 1:
		color = (palette >> 2) & 3;
		break;
	case 2:
		color = (palette >> 4) & 3;
		break;
	case 3:
		color = (palette >> 6) & 3;
		break;
	}
	return color;
}

uint32_t dmg_palette_color(GBContext *gb, uint8_t palette, int palette_index) {
	int color = dmg_palette_index(gb, palette, palette_index);
	if (color <= 3)
		return gb->settings.dmg_palette[color];
	return gb->settings.dmg_palette[3];
}

uint32_t cgb_palette_color(GBContext *gb, uint8_t *palette, int palette_index) {
	uint16_t color = ((uint16_t*)palette)[palette_index];
	/*int red = (color & 0x1F) * 8 + 7;
	int green = ((color & 0x3E0) >> 5) * 8 + 7;
	int blue = ((color & 0x7C00) >> 10) * 8 + 7;
	return red | (green << 8) | (blue << 16);*/
	float red, green, blue, avg, rw, gw, bw;
	uint8_t redb, greenb, blueb;
	red = (float)(color & 0x1F) / 0x1F;
	green = (float)((color & 0x3E0) >> 5) / 0x1F;
	blue = (float)((color & 0x7C00) >> 10) / 0x1F;

	if (gb->settings.emulate_lcd) {
		red = powf(red, 0.8);
		green = powf(green, 0.7);
		blue = powf(blue, 0.8);

		avg = (red+green+blue) / 3;
		rw = red * 0.85f + green * 0.05f + blue * 0.05f + avg * 0.05f;
		gw = red * 0.05f + green * 0.6f + blue * 0.2f + avg * 0.15f;
		bw = red * 0.05f + green * 0.05f + blue * 0.7f + avg * 0.2f;

		red = rw; green = gw; blue = bw;
	}

	redb = (uint8_t)(red*0xFF);
	greenb = (uint8_t)(green*0xFF);
	blueb = (uint8_t)(blue*0xFF);
	return blueb | (greenb << 8) | (redb << 16);
}

void draw_bg(GBContext *gb, uint32_t *line, int scanline, int layer) {
	uint8_t *bgmap = get_bg_tilemap(gb);
	uint8_t *bgattr = get_bg_tileattr(gb);
	uint8_t *tile, *tile_row;
	int win, x, bgx, bgy, wx, wy, bank, tile_index, tilex, tiley, palette_index;
	
	win = gb->io[IO_LCDC] & MASK_LCDC_WINDISP;
	bgy = (uint8_t)(scanline + (int8_t)gb->io[IO_LCD_SCY]) % BG_HEIGHT;
	wx = gb->io[IO_LCD_WX]-7;
	wy = gb->io[IO_LCD_WY];

	for (x = 0; x < DISPLAY_WIDTH; x++) {
		if (win && (x >= wx && scanline >= wy))
			continue;

		bgx = (uint8_t)(x + (int8_t)gb->io[IO_LCD_SCX]) % BG_WIDTH;
		tile_index = (bgy/TILE_HEIGHT)*MAP_WIDTH + (bgx/TILE_WIDTH);
		
		tilex = bgx % TILE_WIDTH;
		tiley = bgy % TILE_HEIGHT;
		bank = gb->cgb_mode ? ((bgattr[tile_index] & MASK_BGATTR_BANK) != 0) : 0;
		if (gb->cgb_mode) {
			if (bgattr[tile_index] & MASK_BGATTR_XFLIP) tilex = TILE_WIDTH-tilex-1;
			if (bgattr[tile_index] & MASK_BGATTR_YFLIP) tiley = TILE_HEIGHT-tiley-1;
		}

		tile = get_tile(gb, bgmap[tile_index], bank);
		tile_row = tile+tiley*2;

		palette_index = ((tile_row[0] & (0x80>>tilex)) != 0) | (((tile_row[1] & (0x80>>tilex)) != 0) << 1);
		if (gb->cgb_mode == 1) {
			if (((bgattr[tile_index] & MASK_BGATTR_PRIORITY) && layer == 2) || ((layer == 0 && palette_index == 0) || (layer == 1 && palette_index > 0)))
				line[x] = cgb_palette_color(gb, gb->cgb_bg_palette+((bgattr[tile_index] & MASK_BGATTR_PAL)*8), palette_index);
		} else if (gb->cgb_mode == 2) {
			if ((palette_index == 0 && layer == 0) || (palette_index > 0 && layer > 0)) {
				palette_index = dmg_palette_index(gb, gb->io[IO_LCD_BGP], palette_index);
				line[x] = cgb_palette_color(gb, gb->cgb_bg_palette+((bgattr[tile_index] & MASK_BGATTR_PAL)*8), palette_index);
			}
		} else {
			if ((palette_index == 0 && layer == 0) || (palette_index > 0 && layer > 0))
				line[x] = dmg_palette_color(gb, gb->io[IO_LCD_BGP], palette_index);
		}
	}
}

void draw_window(GBContext *gb, uint32_t *line, int scanline, int layer) {
	uint8_t *bgmap = get_win_tilemap(gb);
	uint8_t *bgattr = get_win_tileattr(gb);
	uint8_t *tile, *tile_row;
	int x, wx, wy, sx, sy, bank, tile_index, tilex, tiley, palette_index;

	wx = gb->io[IO_LCD_WX]-7;
	wy = gb->io[IO_LCD_WY];
	if (wy > scanline)
		return;

	for (x = 0; x < BG_WIDTH; x++) {
		sx = x+wx;
		sy = scanline-wy;
		if (sx < 0 || sx >= DISPLAY_WIDTH)
			continue;
		tile_index = (sy/TILE_HEIGHT)*MAP_WIDTH + (x/TILE_WIDTH);
		
		tilex = x % TILE_WIDTH;
		tiley = sy % TILE_HEIGHT;
		bank = gb->cgb_mode ? ((bgattr[tile_index] & MASK_BGATTR_BANK) != 0) : 0;
		if (gb->cgb_mode) {
			if (bgattr[tile_index] & MASK_BGATTR_XFLIP) tilex = TILE_WIDTH-tilex-1;
			if (bgattr[tile_index] & MASK_BGATTR_YFLIP) tiley = TILE_HEIGHT-tiley-1;
		}

		tile = get_tile(gb, bgmap[tile_index], bank);
		tile_row = tile+tiley*2;

		palette_index = ((tile_row[0] & (0x80>>tilex)) != 0) | (((tile_row[1] & (0x80>>tilex)) != 0) << 1);
		if (gb->cgb_mode == 1) {
			if (((bgattr[tile_index] & MASK_BGATTR_PRIORITY) && layer == 2) || ((layer == 0 && palette_index == 0) || (layer == 1 && palette_index > 0)))
				line[sx] = cgb_palette_color(gb, gb->cgb_bg_palette+((bgattr[tile_index] & MASK_BGATTR_PAL)*8), palette_index);
		} else if (gb->cgb_mode == 2) {
			if ((palette_index == 0 && layer == 0) || (palette_index > 0 && layer > 0)) {
				palette_index = dmg_palette_index(gb, gb->io[IO_LCD_BGP], palette_index);
				line[sx] = cgb_palette_color(gb, gb->cgb_bg_palette+((bgattr[tile_index] & MASK_BGATTR_PAL)*8), palette_index);
			}
		} else {
			if ((palette_index == 0 && layer == 0) || (palette_index > 0 && layer > 0))
				line[sx] = dmg_palette_color(gb, gb->io[IO_LCD_BGP], palette_index);
		}
	}
}

void draw_sprites(GBContext *gb, uint32_t *line, int scanline, int layer) {
	int sprite, spx, spy, bank, x, spwidth, spheight, tilex, tiley, palette, palette_index;
	uint8_t *data, *tile, *tile_row;

	spwidth = 8;
	spheight = gb->io[IO_LCDC] & MASK_LCDC_OBJSIZE ? 16 : 8;

	//TODO: sprite rightmost priority
	//for (sprite = 0; sprite < 40; sprite++) {
	for (sprite = 39; sprite >= 0; sprite--) {
		data = mem_ptr(gb, 0xFE00+sprite*4, MEM_READ);
		spx = data[1]-8;
		spy = data[0]-16;

		if (scanline >= spy && scanline < spy+spheight) {
			if (gb->cgb_mode) {
				if ((data[3] & MASK_SPATTR_PRIORITY) && layer == 1)
					continue;
				if (!(data[3] & MASK_SPATTR_PRIORITY) && layer == 0)
					continue;
			}

			bank = gb->cgb_mode ? ((data[3] & MASK_SPATTR_BANK) != 0) : 0;

			if (spheight == 8) {
				tile = gb->vram[bank]+(data[2]*TILE_SIZE);
			} else {
				if ((scanline-spy < 8 && !(data[3] & MASK_SPATTR_YFLIP)) || (scanline-spy >= 8 && data[3] & MASK_SPATTR_YFLIP))
					tile = gb->vram[bank]+((data[2]&0xFE)*TILE_SIZE);
				else
					tile = gb->vram[bank]+((data[2]|1)*TILE_SIZE);
			}
			
			palette = (data[3] & MASK_SPATTR_PAL) ? gb->io[IO_LCD_OBP1] : gb->io[IO_LCD_OBP0];

			for (x = spx; x < spx+spwidth; x++) {
				if (x >= 0 && x < DISPLAY_WIDTH) {
					if (data[3] & MASK_SPATTR_XFLIP)
						tilex = TILE_WIDTH - ((x-spx) % TILE_WIDTH) - 1;
					else
						tilex = (x-spx) % TILE_WIDTH;
					if (data[3] & MASK_SPATTR_YFLIP)
						tiley = TILE_HEIGHT - ((scanline-spy) % TILE_HEIGHT) - 1;
					else
						tiley = (scanline-spy) % TILE_HEIGHT;
					tile_row = tile+tiley*2;
					palette_index = ((tile_row[0] & (0x80>>tilex)) != 0) | (((tile_row[1] & (0x80>>tilex)) != 0) << 1);
					if (palette_index != 0) {
						if (gb->cgb_mode == 1) {
							if (data[3] & MASK_SPATTR_PRIORITY) {
								if (layer == 0)
									line[x] = cgb_palette_color(gb, gb->cgb_obj_palette + ((data[3] & MASK_SPATTR_CGBPAL)*8), palette_index);
							} else if (layer > 0) {
								line[x] = cgb_palette_color(gb, gb->cgb_obj_palette + ((data[3] & MASK_SPATTR_CGBPAL)*8), palette_index);
							}
						} else if (gb->cgb_mode == 2) {
							palette_index = dmg_palette_index(gb, palette, palette_index);
							if (data[3] & MASK_SPATTR_PRIORITY) {
								if (layer == 0)
									line[x] = cgb_palette_color(gb, gb->cgb_obj_palette + (((data[3] & MASK_SPATTR_PAL) ? 1 : 0)*8), palette_index);
							} else if (layer > 0) {
								line[x] = cgb_palette_color(gb, gb->cgb_obj_palette + (((data[3] & MASK_SPATTR_PAL) ? 1 : 0)*8), palette_index);
							}
						} else {
							if (data[3] & MASK_SPATTR_PRIORITY) {
								if (layer == 0)
									line[x] = dmg_palette_color(gb, palette, palette_index);
							} else if (layer > 0) {
								line[x] = dmg_palette_color(gb, palette, palette_index);
							}
						}
					}
				}
			}
		}
	}
}

void draw_scanline(GBContext *gb, uint32_t *framebuf, int scanline) {
	uint32_t *line = framebuf+(scanline*DISPLAY_WIDTH);
	int x;
	int bg = gb->io[IO_LCDC] & MASK_LCDC_BGDISP;
	int win = gb->io[IO_LCDC] & MASK_LCDC_WINDISP;
	int obj = gb->io[IO_LCDC] & MASK_LCDC_OBJDISP;
	
	for (x = 0; x < DISPLAY_WIDTH; x++) {
		line[x] = gb->cgb_mode ? 0x00FFFFFF : gb->settings.dmg_palette[0];
	}

	if ((gb->io[IO_LCDC] & MASK_LCDC_ENABLE) == 0)
		return;
	
	if (gb->cgb_mode == 1) { // cgb running cgb games
		if (bg) {
			draw_bg(gb, line, scanline, 0);
			if (win) draw_window(gb, line, scanline, 0);
			if (obj) draw_sprites(gb, line, scanline, 0);
			draw_bg(gb, line, scanline, 1);
			if (win) draw_window(gb, line, scanline, 1);
			if (obj) draw_sprites(gb, line, scanline, 1);
			draw_bg(gb, line, scanline, 2);
			if (win) draw_window(gb, line, scanline, 2);
		} else {
			draw_bg(gb, line, scanline, 0);
			draw_bg(gb, line, scanline, 1);
			if (win) {
				draw_window(gb, line, scanline, 0);
				draw_window(gb, line, scanline, 1);
			}
			if (obj) {
				draw_sprites(gb, line, scanline, 0);
				draw_sprites(gb, line, scanline, 1);
			}
		}
	} else if (gb->cgb_mode == 2) { // cgb running dmg games
		if (bg) draw_bg(gb, line, scanline, 0);
		if (win) draw_window(gb, line, scanline, 0);
		if (obj) draw_sprites(gb, line, scanline, 0);
		if (bg) draw_bg(gb, line, scanline, 1);
		if (win) draw_window(gb, line, scanline, 1);
		if (obj) draw_sprites(gb, line, scanline, 1);
	} else { // dmg
		if (bg) draw_bg(gb, line, scanline, 0);
		if (win) draw_window(gb, line, scanline, 0);
		if (obj) draw_sprites(gb, line, scanline, 0);
		if (bg) draw_bg(gb, line, scanline, 1);
		if (win) draw_window(gb, line, scanline, 1);
		if (obj) draw_sprites(gb, line, scanline, 1);
	}
}