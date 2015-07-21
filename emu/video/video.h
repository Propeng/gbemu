#include "gb.h"

#ifndef VIDEO_H
#define VIDEO_H

#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 144
#define BG_WIDTH 256
#define BG_HEIGHT 256
#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define TILE_SIZE 16
#define MAP_WIDTH 32

#define CLK_MODE0 204
#define CLK_MODE1 4560
#define CLK_MODE2 80
#define CLK_MODE3 172

#define FRAMES_PER_SEC 60

void update_mode(GBContext *gb, int mode);
void video_mode(GBContext *gb, int mode);
void compare_ly(GBContext *gb, int ly);
void update_ly(GBContext *gb, int scanline);
uint8_t* tile_rgb_bitmap (GBContext *gb, int bank, int index);
uint8_t* get_tile(GBContext *gb, uint8_t index, int bank);
uint8_t* get_bg_tilemap(GBContext *gb);
uint8_t* get_win_tilemap(GBContext *gb);
uint8_t* get_bg_tileattr(GBContext *gb);
uint8_t* get_win_tileattr(GBContext *gb);
uint32_t dmg_palette_index(GBContext *gb, uint8_t palette, int palette_index);
uint32_t dmg_palette_color(GBContext *gb, uint8_t palette, int palette_index);
uint32_t cgb_palette_color(GBContext *gb, uint8_t *palette, int palette_index);
void draw_bg(GBContext *gb, uint32_t *line, int scanline, int layer);
void draw_window(GBContext *gb, uint32_t *line, int scanline, int layer);
void draw_sprites(GBContext *gb, uint32_t *line, int scanline, int layer);
void draw_scanline(GBContext *gb, uint32_t *framebuf, int scanline);

void oam_dma(GBContext *gb, uint8_t start);
void hdma_transfer(GBContext *gb, uint8_t attr);
void hdma_hblank(GBContext *gb);

#endif