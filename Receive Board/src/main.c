#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "uart_rx.h"
#include "display.h"

int main(void) {
	UART_Init(9600);
	display_init();
	timer0_init();
	sei();  // enable global interrupts
	
	char buf[32];
	uint16_t distance_cm = 0;
	int16_t temp_c = 0;
	uint8_t angle_deg = 0;
	uint8_t current_sweep = 0;
	uint8_t first_packet = 1;  // Flag for initializing sweep position
	
	while (1) {
		
		// PARSE UART (non-blocking)
		
		if (UART_ReadLine(buf, sizeof(buf)) > 0) {
			size_t len = strlen(buf);
			
			// Strip trailing whitespace
			while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))
			buf[--len] = '\0';
			
			if (len > 0) {
				// Parse: "90g,45c,25C"
				char *tok1 = strtok(buf, ",");  // angle
				char *tok2 = strtok(NULL, ","); // distance
				char *tok3 = strtok(NULL, ","); // temp
				
				// Parse angle (e.g., "90g")
				if (tok1 && tok1[strlen(tok1)-1] == 'g') {
					tok1[strlen(tok1)-1] = '\0';
					long v = strtol(tok1, NULL, 10);
					if (v >= 0 && v <= 180) angle_deg = (uint8_t)v;
				}
				
				// Parse distance in cm (e.g., "45c")
				if (tok2 && tok2[strlen(tok2)-1] == 'c') {
					tok2[strlen(tok2)-1] = '\0';
					long v = strtol(tok2, NULL, 10);
					if (v >= 0 && v <= 300) distance_cm = (uint16_t)v;
					else distance_cm = 0;  // invalid
				}
				
				// Parse temperature (e.g., "25C")
				if (tok3 && tok3[strlen(tok3)-1] == 'C') {
					tok3[strlen(tok3)-1] = '\0';
					long v = strtol(tok3, NULL, 10);
					if (v >= -100 && v <= 150) temp_c = (int16_t)v;
				}
				
				// Update radar state
				display_set_object(angle_deg, distance_cm);
				display_update_readings(distance_cm, temp_c);
				
				// Start sweep at first received angle
				if (first_packet) {
					sweep_angle = angle_deg;
					current_sweep = angle_deg;
					first_packet = 0;
				}
			}
		}
		
		// UPDATE SWEEP DISPLAY (when angle changes)
		
		if (sweep_angle != current_sweep) {
			current_sweep = sweep_angle;
			display_advance_sweep();
		}
	}
	
	return 0;
}