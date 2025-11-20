#define F_CPU 16000000UL 
#include <avr/io.h> 
#include <util/delay.h> 

void uart_init() 
{ 
    UBRR0H = 0; 
    UBRR0L = 103; // 9600 baud 
    UCSR0B = (1 << TXEN0); // enable transmit 
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit packet 
} 

void uart_send(char *data) 
{ 
    while (*data) { // loops until the string ends
        while (!(UCSR0A & (1 << UDRE0))); 
        UDR0 = *data++; // sends another character after the first character is trasmitted
    } 
} 