#ifndef SERVO_ANGLE_H
#define SERVO_ANGLE_H

#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>

void servo_init();
void servo_loop();

extern volatile int angle;

#endif
