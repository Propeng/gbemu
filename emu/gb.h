#include <stdint.h>
#include "peripheral/peripheral.h"

#ifndef GB_H
#define GB_H

#ifdef _MSC_VER
#define inline __inline
#endif

#ifndef NULL
#define NULL 0
#endif

#define reg_af(gb) (gb->registers)
#define reg_bc(gb) (gb->registers+1)
#define reg_de(gb) (gb->registers+2)
#define reg_hl(gb) (gb->registers+3)
#define reg_sp(gb) (gb->registers+4)
#define reg_pc(gb) (gb->registers+5)
#define reg_a(gb) ((uint8_t*)(gb->registers)+1)
#define reg_f(gb) ((uint8_t*)(gb->registers))
#define reg_b(gb) ((uint8_t*)(gb->registers+1)+1)
#define reg_c(gb) ((uint8_t*)(gb->registers+1))
#define reg_d(gb) ((uint8_t*)(gb->registers+2)+1)
#define reg_e(gb) ((uint8_t*)(gb->registers+2))
#define reg_h(gb) ((uint8_t*)(gb->registers+3)+1)
#define reg_l(gb) ((uint8_t*)(gb->registers+3))
//#define reg_n8(gb,n) ((uint8_t*)(gb->registers)+n)
#define reg_n16(gb,n) (gb->registers+n)

#define FLAG_Z 7
#define FLAG_N 6
#define FLAG_H 5
#define FLAG_C 4

#define TITLE_LEN 11

#define BANK_SIZE_8KB 0x2000
#define BANK_SIZE_4KB 0x2000
#define MEM_READ 0
#define MEM_WRITE 1

#define BTN_PRESSED 1
#define BTN_UNPRESSED 0
#define BTN_RIGHT (1<<0)
#define BTN_LEFT (1<<1)
#define BTN_UP (1<<2)
#define BTN_DOWN (1<<3)
#define BTN_A (1<<4)
#define BTN_B (1<<5)
#define BTN_SELECT (1<<6)
#define BTN_START (1<<7)


#define N_CHANNELS 2
#define SND_BUFLEN 2048*N_CHANNELS

static const uint16_t register_defaults_dmg[] = { 0x01B0, 0x0013, 0x00D8, 0x014D, 0xFFFE, 0x0100 };
static const uint16_t register_defaults_cgb[] = { 0x1180, 0x0000, 0xFF56, 0x000D, 0xFFFE, 0x0100 };
static const uint8_t io_defaults1[] = { 0x80, 0xBF, 0xF3, 0x00, 0xBF, 0x00, 0x3F, 0x00, 0x00, 0xBF, 0x7F, 0xFF, 0x9F, 0x00, 0xBF, 0x00, 0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1 };
static const uint8_t io_defaults2[] = { 0x91, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF };
static const uint16_t rst_jump[] = { 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38 };
static const uint16_t int_jump[] = { 0x40, 0x48, 0x50, 0x58, 0x60 };

typedef enum {
	GB_AUTO, GB_FORCE_DMG, GB_FORCE_CGB,
} GBType;

typedef struct {
	GBType hw_type;
	int boot_rom;
	int paused;
	uint32_t dmg_palette[4];
	int emulate_lcd;

	int sample_rate;

	int save_interval;
	void (*save_ram)(void *data);
	void (*play_sound)(float *samples, int n_bytes, void *data);
	void *callback_data;
} GBSettings;

typedef struct {
	int data_len;
	uint8_t *data;
	int rtc_len;
	uint8_t *rtc_data;
} GBBuffer;

typedef struct {
	int playing;
	int restart;
	int counter;
	double freq;
	double duty;
	int length;
	int vol;
	int vol_playing;
	int vol_envolope;
	int vol_direction;
	int sweep_direction;
	int sweep_time;
	int sweep_shift;
	double sweep_delta;
	double sweep_delta_playing;
	int noise_rand;
	int noise_counter;
} GBSoundChannel;

typedef struct {
	GBSettings settings;
	GBPeripheralInfo peripheral;
	uint32_t* last_framebuf;

	uint8_t *rom;
	size_t rom_len;
	char rom_title[TITLE_LEN+1];
	char rom_title_safe[TITLE_LEN+1];
	int cgb_mode;
	int frame_counter;
	uint8_t key_states;
	int has_battery;

	uint8_t cgb_bg_palette[0x40];
	uint8_t cgb_obj_palette[0x40];

	uint8_t dmg_bootrom[0x100];
	uint8_t cgb_bootrom[0x900];

	// hdma
	int hdma_active;
	int hdma_copied;
	int hdma_remaining;

	// mbc
	int cartridge_type;
	int n_rom_banks;
	int mbc_enabled;
	void *mbc_context;
	uint8_t* (*mbc_mem)(void *gb, uint16_t addr, int direction);
	uint8_t (*mbc_filter)(void *gb, uint16_t addr, uint8_t oldval, uint8_t val, int direction);
	void (*mbc_timer)(void *gb);
	void (*mbc_dump)(void *gb, GBBuffer *buf);
	void (*mbc_load)(void *gb, GBBuffer *buf);

	// memory banks
	uint8_t vram[2][BANK_SIZE_8KB];
	uint8_t eram[BANK_SIZE_8KB];
	uint8_t wram[8][BANK_SIZE_4KB];
	uint8_t oam[0xA0];
	uint8_t hram[0x80];
	uint8_t io[0x80];

	// cpu
	int double_speed;
	int halted;
	int error;

	int cycles;
	int extra_cycles;
	int div_counter;
	int tim_counter;
	int timer;

	int enable_interrupts;
	uint8_t int_enable;
	uint16_t registers[6];

	// sound
	float snd_buffer[SND_BUFLEN];
	int snd_ptr;
	double snd_cycles;
	GBSoundChannel channels[4];
	int snd_counter;

	int debug;
} GBContext;

static inline uint8_t* reg_n8(GBContext *gb, int n) {
	switch (n) {
	case 0:
		return reg_a(gb);
		break;
	case 1:
		return reg_f(gb);
		break;
	case 2:
		return reg_b(gb);
		break;
	case 3:
		return reg_c(gb);
		break;
	case 4:
		return reg_d(gb);
		break;
	case 5:
		return reg_e(gb);
		break;
	case 6:
		return reg_h(gb);
		break;
	case 7:
		return reg_l(gb);
		break;
	}
	return 0;
}

GBContext* init_context();
void destroy_context(GBContext *gb);
uint32_t* run_frame(GBContext *gb);

void key_state(GBContext *gb, int key, int state);
void key_mask(GBContext *gb, uint8_t mask);
int get_keys_mask(GBContext *gb);

static inline void set_flag(GBContext *gb, int flag, int value) {
	if (value == 0) {
		*reg_f(gb) &= ~(1 << flag);
	} else {
		*reg_f(gb) |= 1 << flag;
	}
	if (flag == FLAG_C)
		gb->debug = *reg_pc(gb);
}

static inline int read_flag(GBContext *gb, int flag) {
	return (*reg_f(gb) >> flag) & 1;
}

#include "mem.h"
#endif GB_H