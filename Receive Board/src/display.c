#include "include/display.h"
#include <math.h>
#include "st7735.h"
#include <stdio.h>

void display_init(void) {
	ST7735_Init();
	ST7735_FillScreen(ST7735_BLACK);
	display_draw_static_background();
}
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CENTER_X 64
#define CENTER_Y 155
#define MAX_RADIUS 64

void display_draw_static_background(void) {
	// Draw header text side by side
	ST7735_DrawString(5, 5, "Distance:", ST7735_WHITE);
	ST7735_DrawString(80, 5, "Temp:", ST7735_WHITE);  

	// Draw concentric radar arcs
	for (uint8_t r = 4; r <= MAX_RADIUS; r += 20) {
		for (uint8_t angle = 0; angle <= 180; angle += 2) {
			float rad = angle * M_PI / 180.0;
			uint8_t x = CENTER_X + r * cos(rad);
			uint8_t y = CENTER_Y - r * sin(rad);
			ST7735_DrawPixel(x, y, ST7735_GREEN);
		}
	}
char label[2];

// "1" inside arc radius 24, left side
sprintf(label, "1m");
ST7735_DrawString(CENTER_X - 24 + 2, 158 - 7, label, ST7735_WHITE);

// "3" inside arc radius 64, left side
sprintf(label, "3m");
ST7735_DrawString(CENTER_X - 64 + 2, 158 - 7, label, ST7735_WHITE);

// "2" inside arc radius 44, right side
sprintf(label, "2m");
ST7735_DrawString(CENTER_X + 44 - 4, 158 - 7, label, ST7735_WHITE);
}

void display_draw_sweep(uint8_t angle_deg, uint16_t distance_mm) {
	float rad = angle_deg * M_PI / 180.0;

	// Erase previous sweep line (optional)
	static uint8_t prev_angle = 0xFF;
	if (prev_angle != 0xFF) {
		float prev_rad = prev_angle * M_PI / 180.0;
		uint8_t px = CENTER_X + MAX_RADIUS * cos(prev_rad);
		uint8_t py = CENTER_Y - MAX_RADIUS * sin(prev_rad);
		ST7735_DrawLine(CENTER_X, CENTER_Y, px, py, ST7735_BLACK);
	}

	// Draw new sweep line
	uint8_t x1 = CENTER_X + MAX_RADIUS * cos(rad);
	uint8_t y1 = CENTER_Y - MAX_RADIUS * sin(rad);
	ST7735_DrawLine(CENTER_X, CENTER_Y, x1, y1, ST7735_GREEN);
	prev_angle = angle_deg;

	// Draw red dot for object
	uint8_t r = (distance_mm > 3000) ? MAX_RADIUS : distance_mm * MAX_RADIUS / 3000;
	uint8_t xd = CENTER_X + r * cos(rad);
	uint8_t yd = CENTER_Y - r * sin(rad);
	ST7735_DrawPixel(xd, yd, ST7735_RED);
}

void display_update_readings(uint16_t distance_mm, int16_t temp_c) {
	char buffer[16];
	ST7735_DrawString(5, 15, "        ", ST7735_BLACK); // Clear old distance
	ST7735_DrawString(70, 15, "      ", ST7735_BLACK);  // Clear old temp
	sprintf(buffer, "%4dmm", distance_mm);
	ST7735_DrawString(5, 15, buffer, ST7735_WHITE);

	sprintf(buffer, "%2dC", temp_c);
	ST7735_DrawString(70, 15, buffer, ST7735_WHITE);
}