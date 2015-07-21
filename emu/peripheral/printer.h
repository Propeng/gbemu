#include <stdint.h>
#include "peripheral.h"

#ifndef PRINTER_H
#define PRINTER_H

typedef struct {
	uint8_t command;
	uint8_t *data;
	size_t data_len;
	uint8_t alive;
	uint8_t status;
} GBPrinterPacket;

typedef struct {
	uint8_t status;
	
	GBPrinterPacket recv_packet;
	int write_ptr;

	uint8_t *buffer;
	size_t buffer_len;
} GBPrinter;

void init_printer(GBPeripheralInfo *info);
uint8_t printer_transfer(GBPeripheralInfo *info, uint8_t data);

#endif