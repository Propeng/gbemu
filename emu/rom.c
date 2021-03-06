#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "gb.h"
#include "mbc/mbc.h"
#include "sgb.h"

int get_rom_size(int val) {
	switch (val) {
	case 0x00: return 0;
	case 0x01: return 4;
	case 0x02: return 8;
	case 0x03: return 16;
	case 0x04: return 32;
	case 0x05: return 64;
	case 0x06: return 128;
	case 0x07: return 256;
	case 0x52: return 72;
	case 0x53: return 80;
	case 0x54: return 96;
	}
	return 0;
}

int load_rom(GBContext *gb, uint8_t *rom, size_t len) {
	char *ch;
	GBSettings temp_settings;
	GBPeripheralInfo temp_periph;
	uint8_t temp_dmgboot[sizeof(gb->dmg_bootrom)];
	uint8_t temp_sgbboot[sizeof(gb->sgb_bootrom)];
	uint8_t temp_cgbboot[sizeof(gb->cgb_bootrom)];

	memcpy(&temp_settings, &gb->settings, sizeof(GBSettings));
	memcpy(temp_dmgboot, gb->dmg_bootrom, sizeof(gb->dmg_bootrom));
	memcpy(temp_sgbboot, gb->sgb_bootrom, sizeof(gb->sgb_bootrom));
	memcpy(temp_cgbboot, gb->cgb_bootrom, sizeof(gb->cgb_bootrom));
	memcpy(&temp_periph, &gb->peripheral, sizeof(GBPeripheralInfo));
	free(gb->rom);
	free(gb->mbc_context);
	memset(gb, 0, sizeof(GBContext));
	gb->key_states = 0xFF;
	memcpy(&gb->settings, &temp_settings, sizeof(GBSettings));
	memcpy(&gb->peripheral, &temp_periph, sizeof(GBPeripheralInfo));
	memcpy(gb->dmg_bootrom, temp_dmgboot, sizeof(gb->dmg_bootrom));
	memcpy(gb->sgb_bootrom, temp_sgbboot, sizeof(gb->sgb_bootrom));
	memcpy(gb->cgb_bootrom, temp_cgbboot, sizeof(gb->cgb_bootrom));
	
	gb->rom_len = len < 0x150 ? 0x150 : len;
	gb->rom = (uint8_t*)malloc(gb->rom_len);
	memset(gb->rom, 0, gb->rom_len);
	memcpy(gb->rom, rom, len);
	
	memcpy(gb->rom_title, gb->rom+0x134, TITLE_LEN);
	memcpy(gb->rom_title_safe, gb->rom+0x134, TITLE_LEN);
	for (ch = gb->rom_title_safe; *ch != '\0'; ch++) {
		if (*ch == '\\' || *ch == '/') *ch = '_';
	}

	memcpy(gb->io+0x10, io_defaults1, sizeof(io_defaults1));
	memcpy(gb->io+0x40, io_defaults2, sizeof(io_defaults2));

	gb->cgb_mode = gb->rom[0x0143] == 0x80 || gb->rom[0x0143] == 0xC0;
	if (gb->cgb_mode == 0) {
		gb->sgb_mode = gb->rom[0x0146] == 3;
	}

	if (gb->cgb_mode == 0 && gb->sgb_mode == 0) {
		if (gb->settings.dmg_hw == GB_SGB) {
			gb->sgb_mode = 1;
		} else if (gb->settings.dmg_hw == GB_CGB) {
			gb->cgb_mode = 1;
		}
	} else if (gb->cgb_mode == 0 && gb->sgb_mode == 1) {
		if (gb->settings.sgb_hw == GB_DMG) {
			gb->sgb_mode = 0;
		} else if (gb->settings.sgb_hw == GB_CGB) {
			gb->cgb_mode = 1;
			gb->sgb_mode = 0;
		}
	} else if (gb->cgb_mode == 1) {
		if (gb->settings.cgb_hw == GB_DMG) {
			gb->cgb_mode = 0;
		} else if (gb->settings.cgb_hw == GB_SGB) {
			gb->cgb_mode = 0;
			gb->sgb_mode = 1;
		}
	}

	gb->io[IO_LCDSTAT] = gb->cgb_mode ? 0x81 : 0x85;
	if (gb->sgb_mode) sgb_init(gb);
	if (gb->cgb_mode) {
		memcpy(gb->registers, register_defaults_cgb, sizeof(register_defaults_cgb));
		memset(gb->cgb_bg_palette, 0xFF, sizeof(gb->cgb_bg_palette));
		//memset(gb->cgb_obj_palette, 0xFF, sizeof(gb->cgb_obj_palette));
	} else {
		memcpy(gb->registers, register_defaults_dmg, sizeof(register_defaults_dmg));
	}

	if (gb->cgb_mode && !gb->settings.cgb_bootrom && !(gb->rom[0x0143] == 0x80 || gb->rom[0x0143] == 0xC0)) {
		memcpy(gb->cgb_bg_palette, cgb_compat_bg, sizeof(cgb_compat_bg));
		memcpy(gb->cgb_obj_palette, cgb_compat_obj, sizeof(cgb_compat_obj));
		gb->cgb_mode = 2;
	}

	gb->cartridge_type = gb->rom[0x0147];
	gb->n_rom_banks = get_rom_size(gb->rom[0x0149]);

	printf("Running %s, ROM type %02X\n", gb->rom_title, gb->cartridge_type);
	if (!mbc_init(gb)) {
		printf("Unsupported ROM type %02X\n", gb->cartridge_type);
		return 0;
	}

	gb->has_battery = gb->cartridge_type == 0x03 || gb->cartridge_type == 0x06 || gb->cartridge_type == 0x09 || gb->cartridge_type == 0x0D || gb->cartridge_type == 0x0F ||
		gb->cartridge_type == 0x10 || gb->cartridge_type == 0x13 || gb->cartridge_type == 0x17 || gb->cartridge_type == 0x1B || gb->cartridge_type == 0x1E || gb->cartridge_type == 0xFC;
	if (gb->has_battery) printf("External RAM is battery buffered.\n");

	if ((gb->cgb_mode && gb->settings.cgb_bootrom) || (!gb->cgb_mode && !gb->sgb_mode && gb->settings.dmg_bootrom) || (gb->sgb_mode && gb->settings.sgb_bootrom)) {
		*reg_pc(gb) = 0;
		gb->io[IO_BOOTROM] = 0;
	} else {
		gb->io[IO_BOOTROM] = 1;
	}

	printf("Game is running in %s mode.\n", gb->cgb_mode ? "CGB" : (gb->sgb_mode ? "SGB" : "DMG"));

	memset(gb->channels, 0, sizeof(gb->channels));
	gb->io[IO_SND3_WAVE+1] = 0xFF;
	gb->io[IO_SND3_WAVE+3] = 0xFF;
	gb->io[IO_SND3_WAVE+5] = 0xFF;
	gb->io[IO_SND3_WAVE+7] = 0xFF;
	gb->io[IO_SND3_WAVE+9] = 0xFF;
	gb->io[IO_SND3_WAVE+0xB] = 0xFF;
	gb->io[IO_SND3_WAVE+0xD] = 0xFF;
	gb->io[IO_SND3_WAVE+0xF] = 0xFF;

	return 1;
}

void reset_gb(GBContext *gb) {
	uint8_t *rom = (uint8_t*)malloc(gb->rom_len);
	memcpy(rom, gb->rom, gb->rom_len);
	load_rom(gb, rom, gb->rom_len);
	free(rom);
}

/*void reset_gb(GBContext *gb) {
	if (gb->cgb_mode) {
		memcpy(gb->registers, register_defaults_cgb, sizeof(register_defaults_cgb));
		memset(gb->cgb_bg_palette, 0xFF, sizeof(gb->cgb_bg_palette));
	} else {
		memcpy(gb->registers, register_defaults_dmg, sizeof(register_defaults_dmg));
	}
	memset(gb->io, 0, sizeof(gb->io));
	if (gb->settings.boot_rom) {
		*reg_pc(gb) = 0;
		gb->io[IO_BOOTROM] = 0;
	} else {
		gb->io[IO_BOOTROM] = 1;
	}
	memcpy(gb->io+0x10, io_defaults1, sizeof(io_defaults1));
	memcpy(gb->io+0x40, io_defaults2, sizeof(io_defaults2));
	memset(gb->channels, 0, sizeof(gb->channels));
	gb->halted = 0;
	gb->hdma_active = 0;
	gb->double_speed = 0;
}*/