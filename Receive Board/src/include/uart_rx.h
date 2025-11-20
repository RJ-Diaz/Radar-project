#ifndef UART_RX_H
#define UART_RX_H

#include <avr/io.h>

void UART_Init(unsigned long baud);
unsigned char UART_Receive(void);
uint8_t UART_ReadLine(char *buffer, uint8_t maxLen);

#endif