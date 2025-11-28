#include "sensor_sr.h"
#include <util/delay.h>
#include <avr/interrupt.h>

volatile uint16_t echo_start = 0;
volatile uint16_t echo_end = 0;
volatile uint8_t echo_ready = 0;

void hc_sr04_init(void) {
	DDRD |= (1 << PD4);   // TRIG out
	DDRB &= ~(1 << PB0);  // ECHO in

	PCICR  |= (1 << PCIE0);     // enable PCINT0..7 group
	PCMSK0 |= (1 << PCINT0);    // enable PB0 specifically
}

void hc_sr04_trigger(void) {
	PORTD &= ~(1 << PD4);
	_delay_us(2);
	PORTD |= (1 << PD4);
	_delay_us(10);
	PORTD &= ~(1 << PD4);
}

ISR(PCINT0_vect) {
	static uint8_t last = 0;
	uint8_t now = (PINB & (1 << PB0)) ? 1 : 0;

	if (now && !last) {
		echo_start = TCNT1;   // rising edge
		} else if (!now && last) {
		echo_end = TCNT1;     // falling edge
		echo_ready = 1;
	}
	last = now;
}

int hc_sr04_get_distance(void) {
	uint16_t start = echo_start;
	uint16_t end   = echo_end;
	uint16_t ticks = (end >= start) ? (end - start) : (end + (ICR1 - start));
	uint32_t pulse_us = ticks / 2; // prescaler=8 → 0.5 µs per tick
	int dis_cm = pulse_us / 58;
	if (dis_cm < 2 || dis_cm > 400) return -2;
	return dis_cm;
}