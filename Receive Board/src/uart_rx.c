#define F_CPU 16000000UL
#include "uart_rx.h"

void UART_Init(unsigned long baud) {
	unsigned int ubrr = (F_CPU / (16UL * baud)) - 1;
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

unsigned char UART_Receive(void) {
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

// Read until newline or maxLen-1 chars
uint8_t UART_ReadLine(char *buffer, uint8_t maxLen) {
	uint8_t i = 0;
	while (i < maxLen - 1) {
		char c = UART_Receive();
		if (c == '\n' || c == '\r') break;
		buffer[i++] = c;
	}
	buffer[i] = '\0';
	return i;
}