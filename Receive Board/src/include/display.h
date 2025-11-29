#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

// System timing
extern volatile uint32_t system_ticks;
// Sweep state
extern volatile uint8_t sweep_angle;
extern volatile int8_t sweep_dir;
// Initialize display and timer
void display_init(void);
void timer0_init(void);
// Update radar with new sensor reading
void display_set_object(uint8_t angle, uint16_t distance_cm);
// Update HUD text (distance, temp, mode)
void display_update_readings(uint16_t distance_cm, int16_t temp_c);
// Draw current sweep line and any nearby dots
void display_advance_sweep(void);
#endif /* DISPLAY_H_ */