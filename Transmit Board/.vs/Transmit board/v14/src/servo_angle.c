#include "servo_angle.h"
#include <avr/interrupt.h>

volatile int16_t angle = 0;
static int8_t step = +1;
static uint8_t tick_div = 0;

void timer1_init(void) {
	DDRB |= (1 << PB1); // OC1A output
	DDRD |= (1 << PD6); // LED

	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
	ICR1 = 40000;   // 20 ms frame
	OCR1A = 3000;   // neutral pulse (~1.5 ms)
	TIMSK1 |= (1 << TOIE1);
}

ISR(TIMER1_OVF_vect) {
	tick_div++;
	if (tick_div >= 2) {   // sweep slower: update every 2 frames
		tick_div = 0;
		angle += step;
		if (angle >= 180) step = -1;
		else if (angle <= 0) step = +1;
	}
	OCR1A = 544*2 + ((uint32_t)angle * (2400 - 544) * 2) / 180;
}

void servo_set_angle(uint8_t a) {
	if (a > 180) a = 180;
	uint16_t pulse_us = 544 + ((uint32_t)a * (2400 - 544)) / 180;
	OCR1A = pulse_us * 2;
}