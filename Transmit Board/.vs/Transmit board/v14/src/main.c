#include <sensor_mlx.h>
#include <sensor_sr.h>
#include <servo_angle.h>
#include <uart_tx.h>
#include <stdio.h>
#include <util/delay.h>
#define F_CPU 16000000UL


volatile char buffer[32];

int main()
{
    i2c_init();
    hc_sr04_init();
    servo_init();
    uart_init();

    while(1)
    {
        servo_loop();
        hc_sr04_read();
        
        i2c_start();
        i2c_write((slave_address << 1) | 0);
        i2c_write(reg);
        i2c_start();
        i2c_write((slave_address << 1) | 1);

        unsigned char low = i2c_read_ack();
        unsigned char high = i2c_read_nack();

        i2c_stop();

        unsigned short raw = (high << 8) | low;

        temp_f(raw);

        sprintf(buffer, "%dm\n%dg\n%.2fF\n\r\n", distance, angle, temp);
        
        for(int i=0; buffer[i] != '\0'; i++)
        {
            uart_send(buffer[i]);
        }

    }

}