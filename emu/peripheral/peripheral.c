#include <stdint.h>
#include <stdlib.h>
#include "peripheral.h"
#include "printer.h"

void init_peripheral(GBPeripheralInfo *info, GBPeripheralDevice type, peripheral_callback callback, void *user_data) {
	free(info->data);
	info->type = type;
	info->callback = callback;
	info->callback_data = user_data;
	switch (type) {
	case DEVICE_NONE:
		info->data = NULL;
		break;

	case DEVICE_PRINTER:
		init_printer(info);
		break;
	}
}

uint8_t peripheral_transfer(GBPeripheralInfo *info, uint8_t data) {
	switch (info->type) {
	case DEVICE_PRINTER:
		return printer_transfer(info, data);
	}
	return 0;
}