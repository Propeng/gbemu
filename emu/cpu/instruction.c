#include <stdint.h>
#include <stdio.h>
#include "gb.h"
#include "cpu/cpu.h"
#include "cpu/instruction.h"

int run_instruction(GBContext *gb) {
	char errorstr[100];
	uint16_t oldpc = *reg_pc(gb);
	uint8_t opcode = next_byte(gb);
	int opl3 = opcode & 0x7;
	int opl4 = opcode & 0xF;
	int oph4 = opcode >> 4;
	int oph3 = (opcode >> 3) & 0x7;

	void (*bitop)(GBContext *gb, uint8_t *mem, uint8_t val);
	int subop, soph5, soph3, sopl3, sopdir, sopval = 0;
	uint8_t *tmp;

	switch (opcode) {
	case 0x00: //nop
		gb->cycles += 4;
		break;

	case 0x10: //stop 0
		next_byte(gb);
		//gb->halted = 1;
		update_cpu_speed(gb);
		gb->cycles += 4;
		break;

	case 0x76: //halt
		gb->halted = 1;
		gb->cycles += 4;
		break;

	case 0xF3: //di
		gb->enable_interrupts = 0;
		gb->cycles += 4;
		break;

	case 0xFB: //ei
		gb->enable_interrupts = 1;
		gb->cycles += 4;
		break;

	case 0x20: //jr nz,r8
		if (!read_flag(gb, FLAG_Z)) {
			*reg_pc(gb) += (int8_t)(next_byte(gb));
			gb->cycles += 12;
		} else {
			next_byte(gb);
			gb->cycles += 8;
		}
		break;

	case 0x30: //jr nc,r8
		if (!read_flag(gb, FLAG_C)) {
			*reg_pc(gb) += (int8_t)(next_byte(gb));
			gb->cycles += 12;
		} else {
			next_byte(gb);
			gb->cycles += 8;
		}
		break;

	case 0x18: //jr r8
		*reg_pc(gb) += (int8_t)(next_byte(gb));
		gb->cycles += 12;
		break;

	case 0x28: //jr z,r8
		if (read_flag(gb, FLAG_Z)) {
			*reg_pc(gb) += (int8_t)(next_byte(gb));
			gb->cycles += 12;
		} else {
			next_byte(gb);
			gb->cycles += 8;
		}
		break;

	case 0x38: //jr c,r8
		if (read_flag(gb, FLAG_C)) {
			*reg_pc(gb) += (int8_t)(next_byte(gb));
			gb->cycles += 12;
		} else {
			next_byte(gb);
			gb->cycles += 8;
		}
		break;

	case 0x01: case 0x11: case 0x21: case 0x31: //ld bc-sp,d16
		*reg_n16(gb, oph4+1) = next_word(gb);
		gb->cycles += 12;
		break;

	case 0x02: case 0x12: //ld (bc-de),a
		set_mem(gb, *reg_n16(gb, oph4+1), *reg_a(gb));
		gb->cycles += 8;
		break;

	case 0x22: //ldi (hl),a
		set_mem(gb, *reg_hl(gb), *reg_a(gb));
		(*reg_hl(gb))++;
		gb->cycles += 8;
		break;

	case 0x32: //ldd (hl),a
		set_mem(gb, *reg_hl(gb), *reg_a(gb));
		(*reg_hl(gb))--;
		gb->cycles += 8;
		break;

	case 0x0A: case 0x1A: //ld a,(bc-de)
		*reg_a(gb) = read_mem(gb, *reg_n16(gb, oph4+1));
		gb->cycles += 8;
		break;

	case 0x2A: //ldi a,(hl)
		*reg_a(gb) = read_mem(gb, *reg_hl(gb));
		(*reg_hl(gb))++;
		gb->cycles += 8;
		break;

	case 0x3A: //ldd a,(hl)
		*reg_a(gb) = read_mem(gb, *reg_hl(gb));
		(*reg_hl(gb))--;
		gb->cycles += 8;
		break;

	case 0x09: case 0x19: case 0x29: case 0x39: //add hl,bc-sp
		cpu_add16(gb, reg_hl(gb), *reg_n16(gb, oph4+1), 0);
		gb->cycles += 8;
		break;

	case 0x03: case 0x13: case 0x23: case 0x33: //inc bc-sp
		(*reg_n16(gb, oph4+1))++;
		gb->cycles += 8;
		break;

	case 0x0B: case 0x1B: case 0x2B: case 0x3B: //dec bc-sp
		(*reg_n16(gb, oph4+1))--;
		gb->cycles += 8;
		break;

	case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C: //inc b-l
		cpu_inc8(gb, reg_n8(gb, oph3+2));
		gb->cycles += 4;
		break;
		
	case 0x34: //inc (hl)
		cpu_inc_mem(gb, *reg_hl(gb));
		gb->cycles += 12;
		break;

	case 0x3C: //inc a
		cpu_inc8(gb, reg_a(gb));
		gb->cycles += 4;
		break;

	case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D: //dec b-l
		cpu_dec8(gb, reg_n8(gb, oph3+2));
		gb->cycles += 4;
		break;
		
	case 0x35: //dec (hl)
		cpu_dec_mem(gb, *reg_hl(gb));
		gb->cycles += 12;
		break;

	case 0x3D: //dec a
		cpu_dec8(gb, reg_a(gb));
		gb->cycles += 4;
		break;

	case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: //ld b-l,d8
		*reg_n8(gb, oph3+2) = next_byte(gb);
		gb->cycles += 8;
		break;

	case 0x36: //ld (hl),d8
		set_mem(gb, *reg_hl(gb), next_byte(gb));
		gb->cycles += 12;
		break;

	case 0x3E: //ld a,d8
		*reg_a(gb) = next_byte(gb);
		gb->cycles += 8;
		break;

	case 0x07: //rlca
		cpu_rlc(gb, reg_a(gb), 0);
		set_flag(gb, FLAG_Z, 0);
		gb->cycles += 4;
		break;

	case 0x17: //rla
		cpu_rl(gb, reg_a(gb), 0);
		set_flag(gb, FLAG_Z, 0);
		gb->cycles += 4;
		break;

	case 0x0F: //rrca
		cpu_rrc(gb, reg_a(gb), 0);
		set_flag(gb, FLAG_Z, 0);
		gb->cycles += 4;
		break;

	case 0x1F: //rra
		cpu_rr(gb, reg_a(gb), 0);
		set_flag(gb, FLAG_Z, 0);
		gb->cycles += 4;
		break;

	case 0x08: //ld (a16),sp
		set_mem16(gb, next_word(gb), *reg_sp(gb));
		gb->cycles += 20;
		break;

	case 0x27: //daa
		/*set_flag(gb, FLAG_C, 0);
		if (read_flag(gb, FLAG_H) || ((*reg_a(gb)) & 0xF) > 9) {
			*reg_a(gb) += 0x6;
		}
		if (read_flag(gb, FLAG_C) || (*reg_a(gb) >> 4) > 9) {
			*reg_a(gb) += 0x60;
			set_flag(gb, FLAG_C, 1);
		}
		set_flag(gb, FLAG_H, 0);
		set_flag(gb, FLAG_Z, *reg_a(gb) == 0);*/
		cpu_daa(gb, reg_a(gb));
		gb->cycles += 4;
		break;

	case 0x37: //scf
		set_flag(gb, FLAG_N, 0);
		set_flag(gb, FLAG_H, 0);
		set_flag(gb, FLAG_C, 1);
		gb->cycles += 4;
		break;

	case 0x2F: //cpl
		*reg_a(gb) = ~(*reg_a(gb));
		set_flag(gb, FLAG_N, 1);
		set_flag(gb, FLAG_H, 1);
		gb->cycles += 4;
		break;

	case 0x3F: //ccf
		set_flag(gb, FLAG_C, !read_flag(gb, FLAG_C));
		set_flag(gb, FLAG_N, 0);
		set_flag(gb, FLAG_H, 0);
		gb->cycles += 4;
		break;

	// --------------------------------------------------------------------------------
		
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:
	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D:
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D:
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65:
	case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: //ld b-l,b-l
		*reg_n8(gb, oph3+2) = *reg_n8(gb, opl3+2);
		gb->cycles += 4;
		break;

	case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: //ld b-l,(hl)
		*reg_n8(gb, oph3+2) = read_mem(gb, *reg_hl(gb));
		gb->cycles += 8;
		break;

	case 0x47: case 0x4F: case 0x57: case 0x5F: case 0x67: case 0x6F: //ld b-l,a
		*reg_n8(gb, oph3+2) = *reg_a(gb);
		gb->cycles += 4;
		break;
		
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: //ld (hl),b-l
		set_mem(gb, *reg_hl(gb), *reg_n8(gb, opl4+2));
		gb->cycles += 8;
		break;

	case 0x77: //ld (hl),a
		set_mem(gb, *reg_hl(gb), *reg_a(gb));
		gb->cycles += 8;
		break;
		
	case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: //ld a,b-l
		*reg_a(gb) = *reg_n8(gb, opl4-8+2);
		gb->cycles += 4;
		break;

	case 0x7E: //ld a,(hl)
		*reg_a(gb) = read_mem(gb, *reg_hl(gb));
		gb->cycles += 8;
		break;

	case 0x7F: //ld a,a
		*reg_a(gb) = *reg_a(gb);
		gb->cycles += 4;
		break;

	// --------------------------------------------------------------------------------

	case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: //add a,b-l
		cpu_add8(gb, reg_a(gb), *reg_n8(gb, opl4+2), 1);
		gb->cycles += 4;
		break;

	case 0x86: //add a,(hl)
		cpu_add8(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)), 1);
		gb->cycles += 8;
		break;

	case 0x87: //add a,a
		cpu_add8(gb, reg_a(gb), *reg_a(gb), 1);
		gb->cycles += 4;
		break;

	case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: //adc a,b-l
		cpu_adc8(gb, reg_a(gb), *reg_n8(gb, opl4-8+2));
		gb->cycles += 4;
		break;

	case 0x8E: //adc a,(hl)
		cpu_adc8(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)));
		gb->cycles += 8;
		break;

	case 0x8F: //adc a,a
		cpu_adc8(gb, reg_a(gb), *reg_a(gb));
		gb->cycles += 4;
		break;

	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: //sub b-l
		cpu_sub8(gb, reg_a(gb), *reg_n8(gb, opl4+2), 1);
		gb->cycles += 4;
		break;

	case 0x96: //sub (hl)
		cpu_sub8(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)), 1);
		gb->cycles += 8;
		break;

	case 0x97: //sub a
		cpu_sub8(gb, reg_a(gb), *reg_a(gb), 1);
		gb->cycles += 4;
		break;

	case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: //sbc a,b-l
		cpu_sbc8(gb, reg_a(gb), *reg_n8(gb, opl4-8+2));
		gb->cycles += 4;
		break;

	case 0x9E: //sbc a,(hl)
		cpu_sbc8(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)));
		gb->cycles += 8;
		break;

	case 0x9F: //sbc a,a
		cpu_sbc8(gb, reg_a(gb), *reg_a(gb));
		gb->cycles += 4;
		break;

	case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: //and b-l
		cpu_and(gb, reg_a(gb), *reg_n8(gb, opl4+2));
		gb->cycles += 4;
		break;

	case 0xA6: //and (hl)
		cpu_and(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)));
		gb->cycles += 8;
		break;

	case 0xA7: //and a
		cpu_and(gb, reg_a(gb), *reg_a(gb));
		gb->cycles += 4;
		break;

	case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: //xor b-l
		cpu_xor(gb, reg_a(gb), *reg_n8(gb, opl4-8+2));
		gb->cycles += 4;
		break;

	case 0xAE: //xor (hl)
		cpu_xor(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)));
		gb->cycles += 8;
		break;

	case 0xAF: //xor a
		cpu_xor(gb, reg_a(gb), *reg_a(gb));
		gb->cycles += 4;
		break;

	case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: //or b-l
		cpu_or(gb, reg_a(gb), *reg_n8(gb, opl4+2));
		gb->cycles += 4;
		break;

	case 0xB6: //or (hl)
		cpu_or(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)));
		gb->cycles += 8;
		break;

	case 0xB7: //or a
		cpu_or(gb, reg_a(gb), *reg_a(gb));
		gb->cycles += 4;
		break;

	case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: //cp b-l
		cpu_cp8(gb, reg_a(gb), *reg_n8(gb, opl4-8+2), 1);
		gb->cycles += 4;
		break;

	case 0xBE: //cp (hl)
		cpu_cp8(gb, reg_a(gb), read_mem(gb, *reg_hl(gb)), 1);
		gb->cycles += 8;
		break;

	case 0xBF: //cp a
		cpu_cp8(gb, reg_a(gb), *reg_a(gb), 1);
		gb->cycles += 4;
		break;

	// --------------------------------------------------------------------------------

	case 0xC0: //ret nz
		if (!read_flag(gb, FLAG_Z)) {
			*reg_pc(gb) = cpu_pop(gb);
			gb->cycles += 20;
		} else {
			gb->cycles += 8;
		}
		break;

	case 0xD0: //ret nc
		if (!read_flag(gb, FLAG_C)) {
			*reg_pc(gb) = cpu_pop(gb);
			gb->cycles += 20;
		} else {
			gb->cycles += 8;
		}
		break;

	case 0xC1: case 0xD1: case 0xE1: //pop bc-hl
		*reg_n16(gb, oph4-0xC+1) = cpu_pop(gb);
		gb->cycles += 12;
		break;

	case 0xF1: //pop af
		*reg_af(gb) = cpu_pop(gb);
		*reg_f(gb) &= 0xF0;
		gb->cycles += 12;
		break;

	case 0xC5: case 0xD5: case 0xE5: //push bc-hl
		cpu_push(gb, *reg_n16(gb, oph4-0xC+1));
		gb->cycles += 16;
		break;

	case 0xF5: //push af
		cpu_push(gb, *reg_af(gb));
		gb->cycles += 16;
		break;

	case 0xC2: //jp nz,a16
		if (!read_flag(gb, FLAG_Z)) {
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 16;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xD2: //jp nc,a16
		if (!read_flag(gb, FLAG_C)) {
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 16;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xC3: //jp a16
		*reg_pc(gb) = next_word(gb);
		gb->cycles += 16;
		break;

	case 0xC6: //add a,d8
		cpu_add8(gb, reg_a(gb), next_byte(gb), 1);
		gb->cycles += 8;
		break;

	case 0xCE: //adc a,d8
		cpu_adc8(gb, reg_a(gb), next_byte(gb));
		gb->cycles += 8;
		break;

	case 0xD6: //sub d8
		cpu_sub8(gb, reg_a(gb), next_byte(gb), 1);
		gb->cycles += 8;
		break;

	case 0xDE: //sbc a,d8
		cpu_sbc8(gb, reg_a(gb), next_byte(gb));
		gb->cycles += 8;
		break;

	case 0xE6: //and d8
		cpu_and(gb, reg_a(gb), next_byte(gb));
		gb->cycles += 8;
		break;

	case 0xEE: //xor d8
		cpu_xor(gb, reg_a(gb), next_byte(gb));
		gb->cycles += 8;
		break;

	case 0xF6: //or d8
		cpu_or(gb, reg_a(gb), next_byte(gb));
		gb->cycles += 8;
		break;

	case 0xFE: //cp d8
		cpu_cp8(gb, reg_a(gb), next_byte(gb), 1);
		gb->cycles += 8;
		break;

	case 0xCD: //call a16
		cpu_push(gb, (*reg_pc(gb))+2);
		*reg_pc(gb) = next_word(gb);
		gb->cycles += 24;
		break;

	case 0xC4: //call nz,a16
		if (!read_flag(gb, FLAG_Z)) {
			cpu_push(gb, (*reg_pc(gb))+2);
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 24;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xD4: //call nc,a16
		if (!read_flag(gb, FLAG_C)) {
			cpu_push(gb, (*reg_pc(gb))+2);
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 24;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xCC: //call z,a16
		if (read_flag(gb, FLAG_Z)) {
			cpu_push(gb, (*reg_pc(gb))+2);
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 24;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xDC: //call c,a16
		if (read_flag(gb, FLAG_C)) {
			cpu_push(gb, (*reg_pc(gb))+2);
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 24;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF: //rst
		cpu_push(gb, *reg_pc(gb));
		*reg_pc(gb) = rst_jump[oph3];
		gb->cycles += 16;
		break;

	case 0xE0: //ldh (a8),a
		set_mem(gb, next_byte(gb)+0xFF00, *reg_a(gb));
		gb->cycles += 12;
		break;

	case 0xF0: //ldh a,(a8)
		*reg_a(gb) = read_mem(gb, next_byte(gb)+0xFF00);
		gb->cycles += 12;
		break;

	case 0xEA: //ld (a16),a
		set_mem(gb, next_word(gb), *reg_a(gb));
		gb->cycles += 16;
		break;

	case 0xFA: //ld a,(a16)
		*reg_a(gb) = read_mem(gb, next_word(gb));
		gb->cycles += 16;
		break;

	case 0xE2: //ldh (c),a
		set_mem(gb, (*reg_c(gb))+0xFF00, *reg_a(gb));
		gb->cycles += 8;
		break;

	case 0xF2: //ldh a,(c)
		*reg_a(gb) = read_mem(gb, (*reg_c(gb))+0xFF00);
		gb->cycles += 8;
		break;

	case 0xC8: //ret z
		if (read_flag(gb, FLAG_Z)) {
			*reg_pc(gb) = cpu_pop(gb);
			gb->cycles += 20;
		} else {
			gb->cycles += 8;
		}
		break;

	case 0xD8: //ret c
		if (read_flag(gb, FLAG_C)) {
			*reg_pc(gb) = cpu_pop(gb);
			gb->cycles += 20;
		} else {
			gb->cycles += 8;
		}
		break;

	case 0xC9: //ret
		*reg_pc(gb) = cpu_pop(gb);
		gb->cycles += 16;
		break;

	case 0xD9: //reti
		*reg_pc(gb) = cpu_pop(gb);
		/**reg_hl(gb) = cpu_pop(gb);
		*reg_de(gb) = cpu_pop(gb);
		*reg_bc(gb) = cpu_pop(gb);
		*reg_af(gb) = cpu_pop(gb);*/
		gb->enable_interrupts = 1;
		gb->cycles += 16;
		break;
		
	case 0xCA: //jp z,a16
		if (read_flag(gb, FLAG_Z)) {
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 16;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xDA: //jp c,a16
		if (read_flag(gb, FLAG_C)) {
			*reg_pc(gb) = next_word(gb);
			gb->cycles += 16;
		} else {
			next_word(gb);
			gb->cycles += 12;
		}
		break;

	case 0xE8: //add sp,r8
		cpu_add16s(gb, reg_sp(gb), (int8_t)next_byte(gb));
		set_flag(gb, FLAG_Z, 0);
		gb->cycles += 16;
		break;

	case 0xF8: //ld hl,sp+r8
		*reg_hl(gb) = *reg_sp(gb);
		cpu_add16s(gb, reg_hl(gb), (int8_t)next_byte(gb));
		set_flag(gb, FLAG_Z, 0);
		gb->cycles += 12;
		break;

	case 0xF9: //ld sp,hl
		*reg_sp(gb) = *reg_hl(gb);
		gb->cycles += 8;
		break;
	
	case 0xE9: //jp hl
		*reg_pc(gb) = *reg_hl(gb);
		gb->cycles += 4;
		break;

	case 0xCB: //cb prefix
		subop = next_byte(gb);
		soph5 = subop >> 3;
		soph3 = (subop >> 3) & 0x7;
		sopl3 = subop & 0x7;

		switch (soph5) {
		case 0x00>>3: //rlc
			bitop = cpu_rlc;
			sopdir = MEM_READ;
			break;

		case 0x08>>3: //rrc
			bitop = cpu_rrc;
			sopdir = MEM_READ;
			break;

		case 0x10>>3: //rl
			bitop = cpu_rl;
			sopdir = MEM_READ;
			break;

		case 0x18>>3: //rr
			bitop = cpu_rr;
			sopdir = MEM_READ;
			break;

		case 0x20>>3: //sla
			bitop = cpu_sla;
			sopdir = MEM_READ;
			break;

		case 0x28>>3: //sra
			bitop = cpu_sra;
			sopdir = MEM_READ;
			break;

		case 0x30>>3: //swap
			bitop = cpu_swap;
			sopdir = MEM_READ;
			break;

		case 0x38>>3: //srl
			bitop = cpu_srl;
			sopdir = MEM_READ;
			break;

		case 0x40>>3: case 0x48>>3: case 0x50>>3: case 0x58>>3:
		case 0x60>>3: case 0x68>>3: case 0x70>>3: case 0x78>>3: //bit n
			bitop = cpu_bit;
			sopval = soph3;
			sopdir = MEM_READ;
			break;

		case 0x80>>3: case 0x88>>3: case 0x90>>3: case 0x98>>3:
		case 0xA0>>3: case 0xA8>>3: case 0xB0>>3: case 0xB8>>3: //res n
			bitop = cpu_res;
			sopval = soph3;
			sopdir = MEM_WRITE;
			break;

		case 0xC0>>3: case 0xC8>>3: case 0xD0>>3: case 0xD8>>3:
		case 0xE0>>3: case 0xE8>>3: case 0xF0>>3: case 0xF8>>3: //set n
			bitop = cpu_set;
			sopval = soph3;
			sopdir = MEM_WRITE;
			break;

		default:
			gb->error = 1;
			printf(errorstr, "Invalid opcode CB%02Xh at address %04Xh\n", subop, oldpc);
			return subop | 0xCB00;
		}

		switch (sopl3) {
		case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5: //b-l
			bitop(gb, reg_n8(gb, sopl3+2), sopval);
			break;

		case 0x6: //(hl)
			tmp = mem_ptr(gb, *reg_hl(gb), sopdir);
			if (tmp != NULL) {
				bitop(gb, tmp, sopval);
				if (bitop != cpu_bit) set_mem(gb, *reg_hl(gb), *tmp);
			}
			if (bitop == cpu_bit) gb->cycles += 4;
			else gb->cycles += 8;
			break;

		case 0x7: //a
			bitop(gb, reg_a(gb), sopval);
			break;
		}
		gb->cycles += 8;
		break;

	default:
		gb->error = 1;
		printf("Invalid opcode %02Xh at address %04Xh\n", opcode, oldpc);
		if (gb->mbc_context && (uint64_t)(mem_ptr(gb, oldpc, MEM_READ) - gb->rom) != (uint64_t)(oldpc)) {
			printf("Real ROM offset: %01Xh\n", (uint64_t)(mem_ptr(gb, oldpc, MEM_READ) - gb->rom));
		}
		return opcode;
	}

	return 0;
}