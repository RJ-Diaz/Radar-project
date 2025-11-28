#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* --- Public state variables --- */
extern volatile uint32_t system_ticks;   // 1 ms tick counter
extern int sweep_angle;                  // current sweep angle (0–180)
extern int servo_angle;                  // latest servo angle from packet
extern volatile uint16_t obj_distance;   // latest object distance (cm)
extern volatile uint8_t sweep_redraw_pending; // flag set by ISR, consumed in main

void display_init(void);
void display_draw_static_background(void);
void display_set_object(int angle_deg, uint16_t distance_cm);
void display_update_readings(uint16_t distance_cm, int16_t temp_c, int angle_deg);
void display_advance_sweep(void);
void timer0_init(void);

#endif // DISPLAY_H