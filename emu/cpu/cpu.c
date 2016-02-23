#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "gb.h"
#include "cpu/cpu.h"
#include "cpu/instruction.h"
#include "video/video.h"
#include "sound/sound.h"

void update_cpu_speed(GBContext *gb) {
	if (!gb->cgb_mode) return;
	if (gb->io[IO_CPUSPEED] & 1) {
		gb->io[IO_CPUSPEED] ^= 1 | (1 << 7);
		gb->double_speed = !gb->double_speed;
	}
}

int next_interrupt(GBContext *gb) {
	if (interrupt_enabled(gb, INT_VBLANK) && read_interrupt(gb, INT_VBLANK)) {
		return INT_VBLANK;
	} else if (interrupt_enabled(gb, INT_LCDSTAT) && read_interrupt(gb, INT_LCDSTAT)) {
		return INT_LCDSTAT;
	} else if (interrupt_enabled(gb, INT_TIMER) && read_interrupt(gb, INT_TIMER)) {
		//printf("int 50 at pc=%02X, timer=%u\n", *reg_pc(gb), gb->io[IO_TAC] & MASK_TAC_SELECT);
		return INT_TIMER;
	} else if (interrupt_enabled(gb, INT_SERIAL) && read_interrupt(gb, INT_SERIAL)) {
		return INT_SERIAL;
	} else if (interrupt_enabled(gb, INT_JOYPAD) && read_interrupt(gb, INT_JOYPAD)) {
		return INT_JOYPAD;
	}
	return 0;
}

int get_interrupt_jump(GBContext *gb, int flag) {
	switch (flag) {
	case INT_VBLANK:
		return int_jump[0];
	case INT_LCDSTAT:
		return int_jump[1];
	case INT_TIMER:
		return int_jump[2];
	case INT_SERIAL:
		return int_jump[3];
	case INT_JOYPAD:
		return int_jump[4];
	}
	return 0;
}

void do_interrupts(GBContext *gb) {
	int intflag, intjump;
	intflag = next_interrupt(gb);
	if (intflag != 0) {
		if (gb->enable_interrupts) {
			intjump = get_interrupt_jump(gb, intflag);
			gb->enable_interrupts = 0;
			reset_interrupt(gb, intflag);
			
			/*cpu_push(gb, *reg_af(gb));
			cpu_push(gb, *reg_bc(gb));
			cpu_push(gb, *reg_de(gb));
			cpu_push(gb, *reg_hl(gb));*/
			cpu_push(gb, *reg_pc(gb));
			*reg_pc(gb) = intjump;
		} else {
			if (gb->halted) (*reg_pc(gb))++;
		}
		gb->halted = 0;
	}
}

void cpu_cycle(GBContext *gb) {
	if (!gb->halted) {
		//if (*reg_pc(gb) == 0x100)
		//	gb=gb;
		//if (gb->io[IO_BOOTROM]) printf("%04X\n", *reg_pc(gb));
		do_interrupts(gb);
		run_instruction(gb);
	}
}

int t_ticks = 0;

void inc_timers(GBContext *gb, int cycles) {
	int threshold;

	gb->div_counter += cycles;
	while (gb->div_counter >= CLOCKS_PER_DIV) {
		gb->div_counter -= CLOCKS_PER_DIV;
		gb->io[IO_DIV]++;
	}
	
	gb->tim_counter += cycles;
	switch (gb->io[IO_TAC] & MASK_TAC_SELECT) {
	case 0:
		threshold = CLOCKS_PER_TIM0;
		break;
	case 1:
		threshold = CLOCKS_PER_TIM1;
		break;
	case 2:
		threshold = CLOCKS_PER_TIM2;
		break;
	case 3:
		threshold = CLOCKS_PER_TIM3;
		break;
	}

	while (gb->tim_counter >= 4) {
		gb->tim_counter -= 4;
		gb->timer += 4;

		if (gb->timer % threshold == 0 && gb->io[IO_TAC] & MASK_TAC_STOP) {
			gb->io[IO_TIMA]++;
			if (gb->io[IO_TIMA] == 0) {
				gb->io[IO_TIMA] = gb->io[IO_TMA];
				req_interrupt(gb, INT_TIMER);
				//gb->halted = 0;
			}
		}
	}
	//} else {
	//	gb->tim_counter = 0;
	//}
	
	/*
	
	if (gb->io[IO_TAC] & MASK_TAC_STOP) {
		gb->tim_counter += cycles;
		switch (gb->io[IO_TAC] & MASK_TAC_SELECT) {
		case 0:
			threshold = CLOCKS_PER_TIM0;
			break;
		case 1:
			threshold = CLOCKS_PER_TIM1;
			break;
		case 2:
			threshold = CLOCKS_PER_TIM2;
			break;
		case 3:
			threshold = CLOCKS_PER_TIM3;
			break;
		}

		while (gb->tim_counter >= threshold) {
			gb->tim_counter -= threshold;
			gb->io[IO_TIMA]++;
			if (gb->io[IO_TIMA] == 0) {
				gb->io[IO_TIMA] = gb->io[IO_TMA];
				req_interrupt(gb, INT_TIMER);
				gb->halted = 0;
			}
		}
	//} else {
	//	gb->tim_counter = 0;
	}

	*/
}

void serial_transfer(GBContext *gb) {
	int transfer;
	if ((gb->io[IO_SERIAL_CTL] & MASK_SERIAL_START) && gb->peripheral.type != DEVICE_NONE) {
		transfer = peripheral_transfer(&gb->peripheral, gb->io[IO_SERIAL_DATA]);
		if (gb->io[IO_SERIAL_DATA] >= 0) {
			gb->io[IO_SERIAL_DATA] = (uint8_t)transfer;
			req_interrupt(gb, INT_SERIAL);
			gb->io[IO_SERIAL_CTL] &= ~MASK_SERIAL_START;
		}
	}
}

void run_cycles(GBContext *gb, int cycles) {
	int clk_start = gb->cycles;
	int run_until = gb->cycles+cycles-gb->extra_cycles;
	int last_cycles = clk_start;
	do_interrupts(gb);
	/*while (gb->cycles < run_until && !gb->halted) {
		cpu_cycle(gb);
		inc_timers(gb, gb->cycles-last_cycles);
		sound_tick(gb, gb->cycles-last_cycles);
		last_cycles = gb->cycles;
	}
	if (gb->cycles < run_until) {
		inc_timers(gb, run_until-gb->cycles);
		sound_tick(gb, run_until-gb->cycles);
		gb->extra_cycles = 0;
	} else if (gb->cycles > run_until) {
		//printf("%u extra cycles\n", gb->cycles-(clk_start+cycles));
		gb->extra_cycles = gb->cycles-run_until;
	} else {
		gb->extra_cycles = 0;
	}*/
	while (gb->cycles < run_until) {
		if (gb->halted) {
			inc_timers(gb, 4);
			sound_tick(gb, 4);
			gb->cycles += 4;
		} else {
			cpu_cycle(gb);
			inc_timers(gb, gb->cycles-last_cycles);
			sound_tick(gb, gb->cycles-last_cycles);
		}
		
		gb->serial_counter += gb->cycles-last_cycles;
		while (gb->serial_counter >= 100) { //todo: proper serial timing
			gb->serial_counter -= 100;
			serial_transfer(gb);
		}
		
		last_cycles = gb->cycles;
	}
	if (gb->cycles > run_until) {
		gb->extra_cycles = gb->cycles-run_until;
	} else {
		gb->extra_cycles = 0;
	}
}