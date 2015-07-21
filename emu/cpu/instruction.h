#include "gb.h"
#include <assert.h>

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

int run_instruction(GBContext *gb);

static inline uint8_t next_byte(GBContext *gb) {
	uint16_t pc = *reg_pc(gb);
	(*reg_pc(gb))++;
	return read_mem(gb, pc);
}

static inline uint16_t next_word(GBContext *gb) {
	return next_byte(gb) | (next_byte(gb) << 8);
}

// stack

static inline void cpu_push(GBContext *gb, uint16_t val) {
	*reg_sp(gb) -= 2;
	set_mem16(gb, *reg_sp(gb), val);
}

static inline uint16_t cpu_pop(GBContext *gb) {
	uint16_t val = read_mem16(gb, *reg_sp(gb));
	*reg_sp(gb) += 2;
	return val;
}

// binary ops

static inline void cpu_and(GBContext *gb, uint8_t *mem, uint8_t val) {
	*mem &= val;
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 1);
	set_flag(gb, FLAG_C, 0);
}

static inline void cpu_xor(GBContext *gb, uint8_t *mem, uint8_t val) {
	*mem ^= val;
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, 0);
}

static inline void cpu_or(GBContext *gb, uint8_t *mem, uint8_t val) {
	*mem |= val;
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, 0);
}

static inline void cpu_rlc(GBContext *gb, uint8_t *mem, uint8_t val) {
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, *mem >> 7);
	*mem <<= 1;
	*mem |= read_flag(gb, FLAG_C);
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_rrc(GBContext *gb, uint8_t *mem, uint8_t val) {
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, *mem & 1);
	*mem >>= 1;
	*mem |= (read_flag(gb, FLAG_C) << 7);
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_rl(GBContext *gb, uint8_t *mem, uint8_t val) {
	int prevc = read_flag(gb, FLAG_C);
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, *mem >> 7);
	*mem <<= 1;
	*mem |= prevc;
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_rr(GBContext *gb, uint8_t *mem, uint8_t val) {
	int prevc = read_flag(gb, FLAG_C);
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, *mem & 1);
	*mem >>= 1;
	*mem |= prevc << 7;
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_sla(GBContext *gb, uint8_t *mem, uint8_t val) {
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, *mem >> 7);
	*mem <<= 1;
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_sra(GBContext *gb, uint8_t *mem, uint8_t val) {
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, *mem & 1); //TODO: is c always zero
	*mem >>= 1;
	*mem |= (*mem << 1) & 0x80;
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_srl(GBContext *gb, uint8_t *mem, uint8_t val) {
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, *mem & 1);
	*mem >>= 1;
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_swap(GBContext *gb, uint8_t *mem, uint8_t val) {
	*mem = ((*mem) << 4) | ((*mem) >> 4);
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_C, 0);
}

static inline void cpu_bit(GBContext *gb, uint8_t *mem, uint8_t val) {
	set_flag(gb, FLAG_Z, ((*mem) & (1<<val)) == 0);
	set_flag(gb, FLAG_N, 0);
	set_flag(gb, FLAG_H, 1);
}

static inline void cpu_res(GBContext *gb, uint8_t *mem, uint8_t val) {
	*mem &= ~(1 << val);
}

static inline void cpu_set(GBContext *gb, uint8_t *mem, uint8_t val) {
	*mem |= 1 << val;
}

// arithmetic

#include <stdio.h>

//what kind of documentation is this
//http://www.z80.info/z80syntx.htm#DAA
static inline void cpu_daa(GBContext *gb, uint8_t *mem) {
	uint8_t oldval = *mem;
	uint8_t correction = 0;
	
	correction |= read_flag(gb, FLAG_H) ? 6 : 0;
	correction |= read_flag(gb, FLAG_C) ? 0x60 : 0;
	
	if (read_flag(gb, FLAG_N)) {
		*mem -= correction;
	} else {
		correction |= ((*mem) & 0x0F) > 9 ? 6 : 0;
		correction |= (*mem) > 0x99 ? 0x60 : 0;
		*mem += correction;
	}
	if (correction & 0x60) set_flag(gb, FLAG_C, 1);

	set_flag(gb, FLAG_H, 0);
	set_flag(gb, FLAG_Z, *mem == 0);
}

static inline void cpu_inc8(GBContext *gb, uint8_t *mem) {
	set_flag(gb, FLAG_H, (*mem & 0xF) == 0xF);
	(*mem)++;
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
}

static inline void cpu_inc_mem(GBContext *gb, uint16_t addr) {
	set_flag(gb, FLAG_H, (read_mem(gb, addr) & 0xF) == 0xF);
	set_mem(gb, addr, read_mem(gb, addr)+1);
	set_flag(gb, FLAG_Z, read_mem(gb, addr) == 0);
	set_flag(gb, FLAG_N, 0);
}

static inline void cpu_dec8(GBContext *gb, uint8_t *mem) {
	set_flag(gb, FLAG_H, (*mem & 0xF) == 0);
	(*mem)--;
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 1);
}

static inline void cpu_dec_mem(GBContext *gb, uint16_t addr) {
	set_flag(gb, FLAG_H, (read_mem(gb, addr) & 0xF) == 0);
	set_mem(gb, addr, read_mem(gb, addr)-1);
	set_flag(gb, FLAG_Z, read_mem(gb, addr) == 0);
	set_flag(gb, FLAG_N, 1);
}

static inline void cpu_add8(GBContext *gb, uint8_t *mem, uint8_t val, int update_zf) {
	set_flag(gb, FLAG_H, ((*mem) & 0xF) + (val & 0xF) > 0xF);
	set_flag(gb, FLAG_C, *mem + val > 0xFF);
	//printf("add %02X %02X, C=%u H=%u\n", *mem, val, read_flag(gb, FLAG_C), read_flag(gb, FLAG_H));
	*mem += val;
	if (update_zf) set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
}

static inline void cpu_add_mem8(GBContext *gb, uint16_t addr, uint8_t val, int update_zf) {
	set_flag(gb, FLAG_H, (read_mem(gb, addr) & 0xF) + (val & 0xF) > 0xF);
	set_flag(gb, FLAG_C, read_mem(gb, addr) + val > 0xFF);
	set_mem(gb, addr, read_mem(gb, addr)+val);
	if (update_zf) set_flag(gb, FLAG_Z, read_mem(gb, addr) == 0);
	set_flag(gb, FLAG_N, 0);
}

static inline void cpu_add16(GBContext *gb, uint16_t *mem, uint16_t val, int update_zf) {
	//set_flag(gb, FLAG_H, ((*mem) ^ val ^ (*mem+val)) & 0x1000);
	set_flag(gb, FLAG_H, ((*mem) & 0xFFF) + (val & 0xFFF) > 0xFFF);
	set_flag(gb, FLAG_C, *mem + val > 0xFFFF);
	*mem += val;
	if (update_zf) set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
}

static inline void cpu_add16s(GBContext *gb, uint16_t *mem, int8_t val) {
	set_flag(gb, FLAG_H, ((*mem) & 0xF) + (val & 0xF) > 0xF);
	set_flag(gb, FLAG_C, ((*mem) & 0xFF) + (uint8_t)val > 0xFF);
	*mem += val;
	set_flag(gb, FLAG_N, 0);
}

static inline void cpu_sub8(GBContext *gb, uint8_t *mem, uint8_t val, int update_zf) {
	set_flag(gb, FLAG_H, ((*mem) & 0xF) - (val & 0xF) < 0);
	set_flag(gb, FLAG_C, *mem - val < 0);
	*mem -= val;
	if (update_zf) set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 1);
}

static inline void cpu_cp8(GBContext *gb, uint8_t *mem, uint8_t val, int update_zf) {
	set_flag(gb, FLAG_H, ((*mem) & 0xF) - (val & 0xF) < 0);
	set_flag(gb, FLAG_C, *mem - val < 0);
	if (update_zf) set_flag(gb, FLAG_Z, *mem-val == 0);
	set_flag(gb, FLAG_N, 1);
}

static inline void cpu_adc8(GBContext *gb, uint8_t *mem, uint8_t val) {
	int c = read_flag(gb, FLAG_C);
	set_flag(gb, FLAG_H, ((*mem) & 0xF) + (val & 0xF) > 0xF);
	set_flag(gb, FLAG_C, *mem + val + (c?1:0) > 0xFF);
	*mem += val;
	if (c) {
		if (!read_flag(gb, FLAG_H)) set_flag(gb, FLAG_H, ((*mem) & 0xF) == 0xF);
		(*mem)++;
	}
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 0);
}

static inline void cpu_sbc8(GBContext *gb, uint8_t *mem, uint8_t val) {
	int c = read_flag(gb, FLAG_C);
	set_flag(gb, FLAG_H, ((*mem) & 0xF) - (val & 0xF) < 0);
	set_flag(gb, FLAG_C, *mem - val - (c?1:0) < 0);
	*mem -= val;
	if (c) {
		if (!read_flag(gb, FLAG_H)) set_flag(gb, FLAG_H, ((*mem) & 0xF) == 0);
		(*mem)--;
	}
	set_flag(gb, FLAG_Z, *mem == 0);
	set_flag(gb, FLAG_N, 1);
}

#endif