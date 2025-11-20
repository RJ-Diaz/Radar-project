#ifndef SENSOR_MLX_H
#define SENSOR_MLX_H

#include <avr/io.h> 
#include <stdio.h>
#define slave_address 0x5A
#define reg 0x07
#define F_CPU 16000000UL

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(unsigned char data);
unsigned char i2c_read_ack(void);
unsigned char i2c_read_nack(void);
unsigned short mlx_16bit(reg);
float temp_f(unsigned short data);

extern volatile float temp;

#endif 