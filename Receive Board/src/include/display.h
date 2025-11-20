#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>  // Needed for uint8_t, uint16_t

void display_init(void);
void display_draw_static_background(void);
void display_draw_sweep(uint8_t angle_deg, uint16_t distance_mm);
void display_update_readings(uint16_t distance_mm, int16_t temp_c);

#endif