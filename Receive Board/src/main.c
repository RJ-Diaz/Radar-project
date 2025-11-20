#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>

#include "UART_RX.h"
#include "display.h"

int main(void) {
	UART_Init(9600);
	display_init();

	char buf[16];
	uint16_t distance_mm = 0;
	int16_t temp_c = 0;
	uint8_t angle_deg = 0;

	while (1) {
		UART_ReadLine(buf, sizeof(buf));
		uint8_t len = strlen(buf);
		if (len == 0) continue;

		char suffix = buf[len - 1];
		buf[len - 1] = '\0'; // strip suffix
		int value = atoi(buf);

		switch (suffix) {
			case 'm':
			distance_mm = value;
			break;
			case 'F':
			temp_c = value;
			break;
			case 'g':
			angle_deg = value;
			display_draw_sweep(angle_deg, distance_mm);
			break;
			default:
			continue; // ignore unknown suffix
		}

		display_update_readings(distance_mm, temp_c);
	}
}