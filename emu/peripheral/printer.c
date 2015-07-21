#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "printer.h"
#include "peripheral.h"

uint32_t printer_rgb(int color) {
	switch (color) {
	case 0:
		return 0x00F0F0F0;
	case 1:
		return 0x00C0C0C0;
	case 2:
		return 0x00808080;
	case 3:
		return 0x00404040;
	}
	return 0;
}

void decode_picture(GBPeripheralInfo *info, uint8_t *buf, size_t len, uint8_t *options) {
	int width = 160, xtiles = width/8;
	int height = len*4/width, ytiles = height/8;
	int tilex, tiley, x, y;
	uint8_t palette_index, *tile, *pixel;
	uint32_t palette[4];
	uint32_t *picture = (uint32_t*)malloc(width*height*sizeof(uint32_t));
	
	palette[0] = printer_rgb(options[2] & 3);
	palette[1] = printer_rgb((options[2] >> 2) & 3);
	palette[2] = printer_rgb((options[2] >> 4) & 3);
	palette[3] = printer_rgb((options[2] >> 6) & 3);

	/*for (n = 0; n < width*height; n++) {
		pixel = buf+(n/8*2);
		palette_index = ((pixel[0] & (0x80>>(n%8))) != 0) | (((pixel[1] & (0x80>>(n%8))) != 0) << 1);
		picture[n] = palette[palette_index];
	}*/

	for (tiley = 0; tiley < ytiles; tiley++) {
		for (tilex = 0; tilex < xtiles; tilex++) {
			tile = buf+((tiley*xtiles*16)+tilex*16);
			for (y = 0; y < 8; y++) {
				pixel = tile+(y*2);
				for (x = 0; x < 8; x++) {
					palette_index = ((pixel[0] & (0x80>>x)) != 0) | (((pixel[1] & (0x80>>x)) != 0) << 1);
					picture[((tiley*8+y)*width)+tilex*8+x] = palette[palette_index];
				}
			}
		}
	}

	if (info->callback != NULL) info->callback(DEVICE_PRINTER, picture, width, height, info->callback_data);
	//free(picture);
}

uint8_t parse_packet(GBPeripheralInfo *info) {
	GBPrinter *printer = (GBPrinter*)info->data;
	//printf("printer command %02X\n", printer->recv_packet.command);

	switch(printer->recv_packet.command) {
	case 1:
		free(printer->buffer);
		printer->buffer = NULL;
		printer->buffer_len = 0;
		printer->status = 0;
		break;

	case 2:
		printf("Printing...\n");
		decode_picture(info, printer->buffer, printer->buffer_len, printer->recv_packet.data);
		printer->status = 0x04;
		return 0x06;

	case 4:
		printer->buffer = (uint8_t*)realloc(printer->buffer, printer->buffer_len+printer->recv_packet.data_len);
		memcpy(printer->buffer+printer->buffer_len, printer->recv_packet.data, printer->recv_packet.data_len);
		printer->buffer_len += printer->recv_packet.data_len;
		if (printer->buffer_len >= 0x280)
			printer->status = 0x08;
		else
			printer->status = 0x00;
		break;
	}
	return printer->status;
}

void init_printer(GBPeripheralInfo *info) {
	info->data = malloc(sizeof(GBPrinter));
	memset(info->data, 0, sizeof(GBPrinter));
}

uint8_t printer_transfer(GBPeripheralInfo *info, uint8_t data) {
	GBPrinter *printer = (GBPrinter*)info->data;
	int ptr = printer->write_ptr++;
	int status;

	if (ptr == 0) {
		if (data != 0x88)
			printer->write_ptr = 0;
	} else if (ptr == 1) {
		if (data != 0x33)
			printer->write_ptr = 0;
	} else if (ptr == 2) {
		printer->recv_packet.command = data;
		if (data==4)
			data=4;
	} else if (ptr == 3) {
		//compression flag
	} else if (ptr == 4) {
		printer->recv_packet.data_len |= data;
	} else if (ptr == 5) {
		printer->recv_packet.data_len |= data << 8;
		printer->recv_packet.data = (uint8_t*)malloc(printer->recv_packet.data_len);
	} else if (ptr > 5 && ptr <= printer->recv_packet.data_len+5) {
		printer->recv_packet.data[ptr-6] = data;
	} else if (ptr == printer->recv_packet.data_len+6) {
		//checksum lsb
	} else if (ptr == printer->recv_packet.data_len+7) {
		//checksum msb
	} else if (ptr == printer->recv_packet.data_len+8) {
		printer->recv_packet.alive = data;
		return 0x81;
	} else if (ptr == printer->recv_packet.data_len+9) {
		printer->recv_packet.status = data;

		status = parse_packet(info);
		free(printer->recv_packet.data);
		memset(&printer->recv_packet, 0, sizeof(GBPrinterPacket));
		printer->write_ptr = 0;
		return status;
	}

	return 0;
}