#ifndef UART_TX_H
#define UART_TX_H

#define F_CPU 16000000UL 
#include <avr/io.h> 
#include <util/delay.h> 

void uart_init();
void uart_send(char *data);

#endif