#include <stdint.h>

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

typedef enum {
	DEVICE_NONE, DEVICE_PRINTER,
} GBPeripheralDevice;

typedef void (*peripheral_callback)(GBPeripheralDevice device, void *data, int p1, int p2, void *user_data);

typedef struct {
	GBPeripheralDevice type;
	void *data;

	void *callback_data;
	peripheral_callback callback;
} GBPeripheralInfo;

void init_peripheral(GBPeripheralInfo *info, GBPeripheralDevice type, peripheral_callback callback, void *user_data);
int peripheral_transfer(GBPeripheralInfo *info, uint8_t data);

#endif