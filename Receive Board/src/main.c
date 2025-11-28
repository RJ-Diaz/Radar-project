#define F_CPU 16000000UL
#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "uart_rx.h"
#include "display.h"

int main(void) {
	UART_Init(9600);
	display_init();

	timer0_init();            // Timer0 drives sweep timing only
	sei();                    // enable global interrupts

	char buf[32];
	uint16_t distance_cm = 0; // store as centimeters
	float temp_c = 0.0f;
	int angle_deg = 0;

	while (1) {
		// Parse UART packet when available
		if (UART_ReadLine(buf, sizeof(buf)) > 0) {
			size_t len = strlen(buf);
			while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))
			buf[--len] = '\0';

			if (len > 0) {
				char *tok1 = strtok(buf, ",");  // angle (e.g., "90g")
				char *tok2 = strtok(NULL, ","); // distance (e.g., "20c")
				char *tok3 = strtok(NULL, ","); // temp (e.g., "25C")

				// angle
				if (tok1 && tok1[strlen(tok1)-1] == 'g') {
					tok1[strlen(tok1)-1] = '\0';
					long v = strtol(tok1, NULL, 10);
					if (v >= 0 && v <= 180) angle_deg = (int)v;
				}

				// distance in centimeters
				if (tok2 && tok2[strlen(tok2)-1] == 'c') {
					tok2[strlen(tok2)-1] = '\0';
					long v = strtol(tok2, NULL, 10);
					if (v >= 2 && v <= 300) distance_cm = (uint16_t)v;
					else distance_cm = 0; // invalid or out of range
				}

				// temperature (integer °C)
				if (tok3 && tok3[strlen(tok3)-1] == 'C') {
					tok3[strlen(tok3)-1] = '\0';
					long v = strtol(tok3, NULL, 10);
					if (v >= -100 && v <= 150) temp_c = (float)v;
				}

				// update display state
				display_set_object(angle_deg, distance_cm);
				display_update_readings(distance_cm, (int16_t)temp_c, angle_deg);
			}
		}

		// Perform drawing outside ISR when flagged
		if (sweep_redraw_pending) {
			sweep_redraw_pending = 0;
			display_advance_sweep();
		}
	}

	return 0;
}