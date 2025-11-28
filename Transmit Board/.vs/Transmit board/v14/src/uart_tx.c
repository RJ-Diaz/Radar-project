#define F_CPU 16000000UL
#include <avr/io.h>
#include "uart_tx.h"

void uart_init(void) {
	// Baud rate: 9600, UBRR = (F_CPU / (16 * BAUD)) - 1
	// For 16 MHz and 9600 baud → 103
	UBRR0H = 0;
	UBRR0L = 103;

	UCSR0B = (1 << TXEN0);                     // Enable transmitter
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);    // 8-bit data, 1 stop bit, no parity
}

void uart_send_char(char c) {
	while (!(UCSR0A & (1 << UDRE0))); // Wait until buffer empty
	UDR0 = (unsigned char)c;
}

void uart_send(const char *s) {
	while (*s) {
		uart_send_char(*s++);
	}
}