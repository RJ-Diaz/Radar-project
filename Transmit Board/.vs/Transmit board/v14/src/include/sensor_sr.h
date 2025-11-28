#ifndef SENSOR_SR_H
#define SENSOR_SR_H

#include <avr/io.h>

extern volatile uint16_t echo_start;
extern volatile uint16_t echo_end;
extern volatile uint8_t echo_ready;

void hc_sr04_init(void);
void hc_sr04_trigger(void);
int hc_sr04_get_distance(void);

#endif