#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "servo_angle.h"   // contains timer1_init()
#include "sensor_sr.h"     // ultrasonic init + trigger
#include "sensor_mlx.h"    // MLX90614 read functions
#include "uart_tx.h"       // UART init + send

int main(void) {
	// --- Init hardware ---
	hc_sr04_init();
	i2c_init();
	timer1_init();
	uart_init();
	sei(); // enable global interrupts

	int temp_c = 0;
	uint16_t raw = 0;
	uint8_t mlx_counter = 0;
	char pkt[48];

	while (1) {
		// Trigger ultrasonic every ~50 ms
		hc_sr04_trigger();
		_delay_ms(50);

		// Pace MLX reads (every ~250 ms)
		if (++mlx_counter >= 5) {
			mlx_counter = 0;
			if (mlx_read16(0x07, &raw) == 0) {
				temp_c = (int)mlx_temp_c(raw);
			}
		}

		// Only update distance when echo is ready
		int distance_cm = -1;
		if (echo_ready) {
			echo_ready = 0;
			distance_cm = hc_sr04_get_distance();
		}

		// Build packet with current angle (from ISR), distance, and temp
		snprintf(pkt, sizeof(pkt), "%dg,%dc,%dC\n", angle, distance_cm, temp_c);
		uart_send(pkt);
	}
}