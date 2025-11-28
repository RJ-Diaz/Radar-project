#define F_CPU	16000000UL
#include <util/delay.h>
#include "display.h"
#include "st7735.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define CENTER_X 64
#define CENTER_Y 155
#define MAX_RADIUS 64

#define MAX_OBJECTS 8
static int obj_angles[MAX_OBJECTS];
static uint16_t obj_distances[MAX_OBJECTS];
static uint8_t obj_count = 0;


static const uint8_t arc_radii[] = {16, 32, 48, 64};
static const size_t arc_count = sizeof(arc_radii) / sizeof(arc_radii[0]);

volatile uint32_t system_ticks = 0;

int sweep_angle = 0;        // current displayed sweep
int sweep_dir = 1;          // +1 forward, -1 backward
uint8_t sweep_step = 1;     // degrees per update
int prev_angle = -1;

static int target_angle = -1;         // updated by ISR timing
static uint32_t last_update_tick = 0; // tick when packet arrived

static int obj_angle = -1;            // set when sweep crosses servo angle
volatile uint16_t obj_distance = 0;   // centimeters

int servo_angle = 0;

volatile uint8_t sweep_redraw_pending = 0;

static int16_t cos_q8_8[181];
static int16_t sin_q8_8[181];

static inline void safe_pixel(int x, int y, uint16_t color) {
	if ((unsigned)x < 128u && (unsigned)y < 160u)
	ST7735_DrawPixel((uint8_t)x, (uint8_t)y, color);
}

static uint16_t bg_for_radius(int r) {
	for (size_t i = 0; i < arc_count; ++i)
	if ((int)arc_radii[i] == r) return ST7735_GREEN;
	return ST7735_BLACK;
}

static void trig_init(void) {
	// compute once at init to avoid float trig in real-time paths
	for (int a = 0; a <= 180; ++a) {
		double rad = (double)a * 3.14159265358979323846 / 180.0;
		cos_q8_8[a] = (int16_t)lrint(256.0 * cos(rad));
		sin_q8_8[a] = (int16_t)lrint(256.0 * sin(rad));
	}
}
// Persisted dot state (latched)

static uint32_t last_dot_tick = 0;  // moved to global for clarity

static inline int clamp_angle(int a) {
	if (a < 0) return 0;
	if (a > 180) return 180;
	return a;
}
static inline uint8_t dot_active(void) {
	return (obj_angle >= 0 && obj_distance > 0 &&
	(system_ticks - last_dot_tick) < 200);
}

static void erase_radial_column(int angle_deg) {
	if (angle_deg < 0) return;
	angle_deg = clamp_angle(angle_deg);
	int16_t c = cos_q8_8[angle_deg], s = sin_q8_8[angle_deg];

	for (int r = 0; r <= MAX_RADIUS; ++r) {
		int x = CENTER_X + ((r * c) >> 8);
		int y = CENTER_Y - ((r * s) >> 8);
		if ((unsigned)x >= 128u || (unsigned)y >= 160u) continue;
		safe_pixel(x, y, bg_for_radius(r));
	}
	safe_pixel(CENTER_X, CENTER_Y, ST7735_BLACK);
}

/* draw radial column in given color */
static void draw_radial_column(int angle_deg, uint16_t color) {
	angle_deg = clamp_angle(angle_deg);
	int16_t c = cos_q8_8[angle_deg], s = sin_q8_8[angle_deg];
	for (int r = 0; r <= MAX_RADIUS; ++r) {
		int x = CENTER_X + ((r * c) >> 8);
		int y = CENTER_Y - ((r * s) >> 8);
		if ((unsigned)x >= 128u || (unsigned)y >= 160u) continue;
		safe_pixel(x, y, color);
	}
}

void timer0_init(void) {
	TCCR0A = (1 << WGM01);                 // CTC
	TCCR0B = (1 << CS01) | (1 << CS00);    // prescaler=64
	OCR0A = 249;                           // 16MHz/64/250 = 1000 Hz
	TIMSK0 = (1 << OCIE0A);                // enable compare A interrupt
}


static void draw_red_dot_at(int angle_deg, uint16_t distance_cm) {
	if (distance_cm == 0 || distance_cm > 100) return;
	angle_deg = clamp_angle(angle_deg);

	int16_t c = cos_q8_8[angle_deg], s = sin_q8_8[angle_deg];
	int r_scaled = (int)((long)distance_cm * MAX_RADIUS / 100L);

	int xd = CENTER_X + ((r_scaled * c) >> 8);
	int yd = CENTER_Y - ((r_scaled * s) >> 8);

	// perpendicular offset ~2 px
	int nx = (2 * s) / 256;
	int ny = (2 * c) / 256;

	int xdot = xd + nx;
	int ydot = yd + ny;

	for (int dx = -2; dx <= 1; ++dx) {
		for (int dy = -2; dy <= 1; ++dy) {
			int px = xdot + dx, py = ydot + dy;
			if ((unsigned)px < 128u && (unsigned)py < 160u)
			safe_pixel(px, py, ST7735_RED);
		}
	}
}

void display_init(void) {
	ST7735_Init();
	ST7735_FillScreen(ST7735_BLACK);
	trig_init();
	// static background
	ST7735_DrawString(5, 5,  "Distance:", ST7735_WHITE);
	ST7735_DrawString(80, 5, "Temp:",     ST7735_WHITE);

	for (size_t i = 0; i < arc_count; ++i) {
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

void display_draw_static_background(void) {
	// retained for API completeness (already drawn in display_init)
}

void display_set_object(int angle, uint16_t distance_cm) {
	if (obj_count < MAX_OBJECTS) {
		obj_angles[obj_count] = clamp_angle(angle);
		obj_distances[obj_count] = distance_cm;
		obj_count++;
		} else {
		// overwrite oldest if buffer full
		for (uint8_t i = 1; i < MAX_OBJECTS; i++) {
			obj_angles[i-1] = obj_angles[i];
			obj_distances[i-1] = obj_distances[i];
		}
		obj_angles[MAX_OBJECTS-1] = clamp_angle(angle);
		obj_distances[MAX_OBJECTS-1] = distance_cm;
	}
	last_update_tick = system_ticks;
}

void display_update_readings(uint16_t distance_cm, int16_t temp_c, int angle_deg) {
	char buffer[16];

	// clear small HUD fields
	ST7735_DrawRect(5,  15, 60,  8, ST7735_BLACK);
	ST7735_DrawRect(70, 15, 50,  8, ST7735_BLACK);
	ST7735_DrawRect(40, 40, 60, 12, ST7735_BLACK);

	sprintf(buffer, "%4dcm", distance_cm);
	ST7735_DrawString(5,  15, buffer, ST7735_WHITE);

	sprintf(buffer, "%2dC", temp_c);
	ST7735_DrawString(70, 15, buffer, ST7735_WHITE);

	if (target_angle >= 0 && (system_ticks - last_update_tick) < 1000) {
		ST7735_DrawString(40, 40, "FOLLOW", ST7735_GREEN);
		} else {
		ST7735_DrawString(40, 40, "AUTO", ST7735_YELLOW);
	}
}

static void display_debug_overlay(void) {
	char buffer[32];
	ST7735_DrawRect(20, 60, 88, 24, ST7735_BLACK);

	sprintf(buffer, "S:%3d", sweep_angle);
	ST7735_DrawString(20, 60, buffer, ST7735_WHITE);

	sprintf(buffer, "T:%3d", target_angle);
	ST7735_DrawString(20, 70, buffer, ST7735_GREEN);

	sprintf(buffer, "O:%3d", obj_angle);
	ST7735_DrawString(20, 80, buffer, ST7735_RED);
	
	char d[12]; sprintf(d, "D:%3d", obj_distance);
	ST7735_DrawRect(70, 70, 30, 8, ST7735_BLACK);
	ST7735_DrawString(70, 70, d, ST7735_WHITE);

}

void display_advance_sweep(void) {
	static int last_drawn_angle = 0;
	static uint32_t last_dot_tick = 0;
	const int angle_tol = 2;
	const uint32_t dot_ms = 200;

	if (last_drawn_angle >= 0)
	erase_radial_column(last_drawn_angle);

	sweep_angle = clamp_angle(sweep_angle);
	draw_radial_column(sweep_angle, ST7735_WHITE);

	int best_idx = -1, best_diff = 999;
	for (uint8_t i = 0; i < obj_count; i++) {
		int diff = abs(sweep_angle - clamp_angle(obj_angles[i]));
		if (diff < best_diff) { best_diff = diff; best_idx = i; }
	}

	if (best_idx >= 0 && obj_distances[best_idx] > 0) {
		int candidate_angle = clamp_angle(obj_angles[best_idx]);
		if (abs(sweep_angle - candidate_angle) <= angle_tol) {
			obj_angle = candidate_angle;
			obj_distance = obj_distances[best_idx];
			draw_red_dot_at(obj_angle, obj_distance);
			last_dot_tick = system_ticks;
		}
	}

	if ((system_ticks - last_dot_tick) < dot_ms && obj_angle >= 0 && obj_distance > 0)
	draw_red_dot_at(obj_angle, obj_distance);

	last_drawn_angle = sweep_angle;
	prev_angle = sweep_angle;

	display_debug_overlay();
}
/* Timer0 ISR: constant-time, only updates sweep state and sets flag */
ISR(TIMER0_COMPA_vect) {
	system_ticks++;

	static uint8_t tick_count = 0;
	tick_count++;
	if (tick_count >= 160) {   
		tick_count = 0;

		sweep_angle += sweep_dir * sweep_step;
		if (sweep_angle >= 180) { sweep_angle = 180; sweep_dir = -1; }
		if (sweep_angle <= 0)   { sweep_angle = 0;   sweep_dir = 1; }

		target_angle = sweep_angle;     // keep T updated
		sweep_redraw_pending = 1;       // signal main to draw
	}
}