#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>

void servo_init()
{
    DDRD |= (1<<7); 
    TCCR1A = (1<<COM1A1) | (1<<WGM11); // Fast PWM, non-in
    TCCR1B = 0x1A; // prescaler = 8
    ICR1A = 39999; // sets duty cycle

}

volatile int angle = 0;
void servo_loop()
{
    for (int pulse = 1500; pulse <= 5500; pulse += 10)
    {
        OCR1A = pulse;
        _delay_ms(5);
        angle = (pulse-1500)*(180/4000);

    }
    for (int pulse = 5500; pulse >= 1500; pulse -= 10)
    {
        OCR1A = pulse;
        _delay_ms(5);
        angle = (pulse-1500)*(180/4000);

    }
}