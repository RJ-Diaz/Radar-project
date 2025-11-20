#define F_CPU 16000000UL
#define slave_address
#define object_temp_code

#include <avr/io.h> 
#include <stdio.h>
#include <util/delay.h>

void i2c_init(void)
{
    TWSR = 0x00; // no prescaler
    TWBR = 72; // sets clock frequency to 100kHz
    TWCR = (1<<TWEN); // enables 

}

volatile float temp = 0;

void i2c_start(void)
{
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while(!(TWCR & (1<<TWINT)));

}

void i2c_stop(void)
{
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
    _delay_ms(2);
}

void i2c_write(unsigned char data)
{
    TWDR = data; // input the data in the register
    TWCR = (1<<TWINT) | (1<<TWEN); // intializes start
    while((TWCR & (1<<TWINT)) == 0); // waits for all data to be transmitted

}

unsigned char i2c_read_ack(void)
{
 
        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
        while((TWCR & (1<<TWINT)) == 0);
        return TWDR;
}

unsigned char i2c_read_nack(void)
{
    TWCR = (1<<TWINT) | (1<<TWEN);
    while((TWCR & (1<<TWINT)) == 0);
    return TWDR;
}

unsigned short mlx_16bit(unsigned char reg)
{

    unsigned short data;

    i2c_start();
    i2c_write((slave_address << 1) | 0);
    i2c_write(reg);
    i2c_start();
    i2c_write((slave_address << 1) | 1); 

    unsigned char low = i2c_read_ack();
    unsigned char high = i2c_read_nack();

    i2c_stop();

    data = ((high << 8) | low);
    return data;


}

float temp_f(unsigned short data)
{
    temp = (((data * 0.02) - 273.15)* (9.0/5.0)) + 32;
    return temp;
}
