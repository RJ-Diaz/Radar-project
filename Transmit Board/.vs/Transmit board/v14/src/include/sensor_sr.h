
#ifndef SENSOR_SR_H
#define SENSOR_SR_H

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#define F_CPU 16000000UL
#define trig PD4 
#define echo PD5 

void hc_sr04_init();
float hc_sr04_read();

extern volatile float distance;

#endif