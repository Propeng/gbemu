#include "gb.h"

#ifndef CPU_H
#define CPU_H

#define CLOCKS_PER_SEC 4213440
#define CLOCKS_PER_DIV 256
#define CLOCKS_PER_TIM0 1024
#define CLOCKS_PER_TIM1 16
#define CLOCKS_PER_TIM2 64
#define CLOCKS_PER_TIM3 256

#define INT_VBLANK (1<<0)
#define INT_LCDSTAT (1<<1)
#define INT_TIMER (1<<2)
#define INT_SERIAL (1<<3)
#define INT_JOYPAD (1<<4)

#define interrupt_enabled(gb,i) ((gb->int_enable & i) != 0)
#define read_interrupt(gb,i) ((gb->io[IO_INTFLAG] & i) != 0)
#define req_interrupt(gb,i) gb->io[IO_INTFLAG] |= i;
#define reset_interrupt(gb,i) gb->io[IO_INTFLAG] &= ~i;

void update_cpu_speed(GBContext *gb);
int next_interrupt(GBContext *gb);
int get_interrupt_jump(GBContext *gb, int flag);
void do_interrupts(GBContext *gb);
void cpu_cycle(GBContext *gb);
void inc_timers(GBContext *gb, int cycles);
void run_cycles(GBContext *gb, int cycles);

#endif