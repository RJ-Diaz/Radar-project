#define F_CPU 16000000UL
#define echo
#define trig
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

void hc_sr04_init() 
{ 
    DDRD |= (1 << trig);   // TRIG as output 
    DDRD &= ~(1 << echo);  // ECHO as input 
} 

volatile float distance = 0;

float hc_sr04_read() 
{ 
    TIFR0 |= (1<<TOV0);

    PORTD &= ~(1 << trig); 
    _delay_us(2); // sets trigger to low before starting
    
    PORTD |= (1 << trig); 
    _delay_us(10); // sends a 10us high pulse to tell the sensor to start measurement

    PORTD &= ~(1 << trig); 


    // Wait for echo HIGH 
    TCNT0 = 0; 
    TCCR0B = (1 << CS02) | (1<<CS00); // Start timer (prescaler 1024) 

    while (!(PIND & (1 << echo)))        // Wait for echo LOW 
    {

        if (TIFR0 & (1<<TOV0)){
        TCCR0B = 0;
        return -1;
        }
    }

    TCNT0 = 0;
    TIFR0 |= (1<<TOV0);

    while((PIND & (1<<echo)))
    {
        if (TIFR0 & (1<<TOV0))
        {
            TCCR0B = 0;
            return -1;
        }
    }

    TCCR0B = 0;

    distance = (TCNT0 * 1.0976);
    return distance; // Convert to cm 
} 


 