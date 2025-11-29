#define F_CPU 16000000UL
#include <util/delay.h>
#include "display.h"
#include "st7735.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define CENTER_X 64
#define CENTER_Y 155
#define MAX_RADIUS 64
#define ANGLE_BUCKETS 181


// GLOBAL STATE

volatile uint32_t system_ticks = 0;
volatile uint8_t sweep_angle = 0;
volatile int8_t sweep_dir = 1;

// Spatial storage: one distance per angle (0 = no object)
static uint16_t distance_map[ANGLE_BUCKETS];

// Lightweight averaging: only store previous value per angle
static uint16_t distance_prev[ANGLE_BUCKETS];

// Pre-computed trig tables (Q8.8 fixed point)
static int16_t cos_q8_8[181];
static int16_t sin_q8_8[181];


// HELPERS


static inline void safe_pixel(int x, int y, uint16_t color) {
	if ((unsigned)x < 128u && (unsigned)y < 160u)
	ST7735_DrawPixel((uint8_t)x, (uint8_t)y, color);
}

static uint16_t bg_for_radius(int r) {
	if (r == 16 || r == 32 || r == 48 || r == 64)
	return ST7735_GREEN;
	return ST7735_BLACK;
}

static void trig_init(void) {
	for (int a = 0; a <= 180; ++a) {
		double rad = (double)a * 3.14159265358979323846 / 180.0;
		cos_q8_8[a] = (int16_t)lrint(256.0 * cos(rad));
		sin_q8_8[a] = (int16_t)lrint(256.0 * sin(rad));
	}
}

static inline uint8_t clamp_angle(int a) {
	if (a < 0) return 0;
	if (a > 180) return 180;
	return a;
}


// INITIALIZATION

void timer0_init(void) {
	TCCR0A = (1 << WGM01);                 // CTC
	TCCR0B = (1 << CS01) | (1 << CS00);    // prescaler=64
	OCR0A = 249;                           // 16MHz/64/250 = 1000 Hz
	TIMSK0 = (1 << OCIE0A);                // enable compare A interrupt
}

void display_init(void) {
	ST7735_Init();
	ST7735_FillScreen(ST7735_BLACK);
	trig_init();
	
	// Clear distance maps
	memset(distance_map, 0, sizeof(distance_map));
	memset(distance_prev, 0, sizeof(distance_prev));
	
	// Static background
	ST7735_DrawString(5, 5,  "Distance:", ST7735_WHITE);
	ST7735_DrawString(80, 5, "Temp:",     ST7735_WHITE);
	
	// Draw range arcs
	static const uint8_t arc_radii[] = {16, 32, 48, 64};
	for (size_t i = 0; i < 4; ++i) {
		uint8_t r = arc_radii[i];
		for (uint8_t a = 0; a <= 180; a += 2) {
			int16_t c = cos_q8_8[a], s = sin_q8_8[a];
			int x = CENTER_X + ((r * c) >> 8);
			int y = CENTER_Y - ((r * s) >> 8);
			if ((unsigned)x < 128u && (unsigned)y < 160u)
			ST7735_DrawPixel((uint8_t)x, (uint8_t)y, ST7735_GREEN);
		}
	}
	ST7735_DrawString(CENTER_X - 64, 158 - 7, "1m", ST7735_WHITE);
}

// CORE RADAR FUNCTIONS

void display_set_object(uint8_t angle, uint16_t distance_cm) {
	angle = clamp_angle(angle);
	
	if (distance_cm > 0 && distance_cm <= 100) {
		// Simple 2-sample average to smooth noise
		uint16_t prev = distance_prev[angle];
		if (prev > 0) {
			distance_map[angle] = (distance_cm + prev) / 2;
			} else {
			distance_map[angle] = distance_cm;
		}
		distance_prev[angle] = distance_cm;
		} else {
		// Clear invalid readings
		distance_map[angle] = 0;
		distance_prev[angle] = 0;
	}
}

void display_update_readings(uint16_t distance_cm, int16_t temp_c) {
	char buffer[16];
	
	// Clear and update distance
	ST7735_DrawRect(5,  15, 60,  8, ST7735_BLACK);
	sprintf(buffer, "%4dcm", distance_cm);
	ST7735_DrawString(5,  15, buffer, ST7735_WHITE);
	
	// Clear and update temp
	ST7735_DrawRect(70, 15, 50,  8, ST7735_BLACK);
	sprintf(buffer, "%2dC", temp_c);
	ST7735_DrawString(70, 15, buffer, ST7735_WHITE);
}

void display_advance_sweep(void) {
	static uint8_t last_angle = 255;  // 255 = not yet drawn
	
	// Skip if angle hasn't changed
	if (last_angle == sweep_angle) {
		return;
	}
	
	
	// ERASE DOTS ON REVERSE SWEEP
	
	if (sweep_dir < 0) {  // Moving backward (180 -> 0)
		// Clear dots that we're sweeping back over (both data AND pixels)
		for (int offset = -5; offset <= 5; offset++) {
			int clear_angle = sweep_angle + offset;
			if (clear_angle < 0 || clear_angle > 180) continue;
			
			// Get the distance before clearing it
			uint16_t dist = distance_map[clear_angle];
			if (dist > 0) {
				// Erase the dot pixels
				int r_scaled = (int)((long)dist * MAX_RADIUS / 100L);
				int16_t obj_c = cos_q8_8[clear_angle];
				int16_t obj_s = sin_q8_8[clear_angle];
				
				int xd = CENTER_X + ((r_scaled * obj_c) >> 8);
				int yd = CENTER_Y - ((r_scaled * obj_s) >> 8);
				
				// Erase 4x4 area (slightly bigger than the 3x3 dot)
				for (int dx = -2; dx <= 2; dx++) {
					for (int dy = -2; dy <= 2; dy++) {
						int px = xd + dx;
						int py = yd + dy;
						if ((unsigned)px < 128u && (unsigned)py < 160u) {
							// Calculate radius to restore proper background
							int rx = px - CENTER_X;
							int ry = CENTER_Y - py;
							int r = (int)sqrt(rx*rx + ry*ry);
							safe_pixel(px, py, bg_for_radius(r));
						}
					}
				}
			}
			
			// Clear the data
			distance_map[clear_angle] = 0;
		}
	}
	
	
	// ERASE PREVIOUS SWEEP (3-degree swath for clean trails)
	
	if (last_angle <= 180) {
		for (int offset = -1; offset <= 1; offset++) {
			int a = last_angle + offset;
			if (a < 0 || a > 180) continue;
			
			int16_t c = cos_q8_8[a], s = sin_q8_8[a];
			for (int r = 0; r <= MAX_RADIUS; ++r) {
				int x = CENTER_X + ((r * c) >> 8);
				int y = CENTER_Y - ((r * s) >> 8);
				if ((unsigned)x >= 128u || (unsigned)y >= 160u) continue;
				safe_pixel(x, y, bg_for_radius(r));
			}
		}
		safe_pixel(CENTER_X, CENTER_Y, ST7735_BLACK);
	}
	
	// DRAW CURRENT SWEEP LINE
	
	uint8_t angle = clamp_angle(sweep_angle);
	int16_t c = cos_q8_8[angle], s = sin_q8_8[angle];
	for (int r = 0; r <= MAX_RADIUS; ++r) {
		int x = CENTER_X + ((r * c) >> 8);
		int y = CENTER_Y - ((r * s) >> 8);
		if ((unsigned)x >= 128u || (unsigned)y >= 160u) continue;
		safe_pixel(x, y, ST7735_WHITE);
	}
	
	// DRAW DOTS FOR OBJECTS NEAR SWEEP (±5 degree window)
	
	for (int offset = -5; offset <= 5; offset++) {
		int check_angle = angle + offset;
		if (check_angle < 0 || check_angle > 180) continue;
		
		uint16_t dist = distance_map[check_angle];
		if (dist == 0) continue;  // No object at this angle
		
		// Scale distance to screen radius
		int r_scaled = (int)((long)dist * MAX_RADIUS / 100L);
		int16_t obj_c = cos_q8_8[check_angle];
		int16_t obj_s = sin_q8_8[check_angle];
		
		int xd = CENTER_X + ((r_scaled * obj_c) >> 8);
		int yd = CENTER_Y - ((r_scaled * obj_s) >> 8);
		
		// Draw 3x3 red dot
		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				int px = xd + dx;
				int py = yd + dy;
				if ((unsigned)px < 128u && (unsigned)py < 160u)
				safe_pixel(px, py, ST7735_RED);
			}
		}
	}
	
	last_angle = sweep_angle;
}

// INTERRUPT (Sweep Timer)

ISR(TIMER0_COMPA_vect) {
	system_ticks++;
	
	static uint8_t tick_count = 0;
	tick_count++;
	if (tick_count >= 40) {   // Update sweep every 40ms
		tick_count = 0;
		
		sweep_angle += sweep_dir;
		if (sweep_angle >= 180) { sweep_angle = 180; sweep_dir = -1; }
		if (sweep_angle == 0)   { sweep_dir = 1; }
	}
}