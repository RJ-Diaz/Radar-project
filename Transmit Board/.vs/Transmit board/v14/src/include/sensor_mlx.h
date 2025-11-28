#ifndef SENSOR_MLX_H
#define SENSOR_MLX_H

#include <avr/io.h>
#include <stdint.h>

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(uint8_t data);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);

int mlx_read16(uint8_t reg, uint16_t *out);
float mlx_temp_c(uint16_t raw);
float mlx_temp_f(uint16_t raw);
uint8_t crc8(uint8_t *data, uint8_t len);

#endif