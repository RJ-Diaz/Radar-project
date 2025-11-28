#ifndef SERVO_ANGLE_H
#define SERVO_ANGLE_H

#include <avr/io.h>

extern volatile int16_t angle;

void timer1_init(void);
void servo_set_angle(uint8_t a);

#endif