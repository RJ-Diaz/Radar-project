#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "sensor_mlx.h"

#define MLX90614_ADDR 0x5A

void i2c_init(void) {
	TWSR = 0x00;
	TWBR = 72;
	TWCR = (1 << TWEN);
}

void i2c_start(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

void i2c_stop(void) {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
	_delay_ms(2);
}

void i2c_write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

uint8_t i2c_read_ack(void) {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

uint8_t i2c_read_nack(void) {
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

int mlx_read16(uint8_t reg, uint16_t *out) {
	uint8_t low, high, pec;
	uint8_t buf[5];

	i2c_start();
	i2c_write((MLX90614_ADDR << 1) | 0);
	if ((TWSR & 0xF8) != 0x18) return -1;

	i2c_write(reg);
	if ((TWSR & 0xF8) != 0x28) return -2;

	i2c_start();
	i2c_write((MLX90614_ADDR << 1) | 1);
	if ((TWSR & 0xF8) != 0x40) return -3;

	low  = i2c_read_ack();
	high = i2c_read_ack();
	pec  = i2c_read_nack();
	i2c_stop();

	buf[0] = (MLX90614_ADDR << 1) | 0;
	buf[1] = reg;
	buf[2] = (MLX90614_ADDR << 1) | 1;
	buf[3] = low;
	buf[4] = high;

	if (crc8(buf, 5) != pec) return -4;

	*out = ((uint16_t)high << 8) | low;
	return 0;
}

float mlx_temp_c(uint16_t raw) {
	return (raw * 0.02f) - 273.15f;
}

float mlx_temp_f(uint16_t raw) {
	return ((raw * 0.02f) - 273.15f) * 9.0f / 5.0f + 32.0f;
}

uint8_t crc8(uint8_t *data, uint8_t len) {
	uint8_t crc = 0;
	for (uint8_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
		}
	}
	return crc;
}