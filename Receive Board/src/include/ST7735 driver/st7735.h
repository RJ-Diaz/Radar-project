#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>

// Color definitions
#define ST7735_BLACK   0x0000
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_BLUE    0x001F
#define ST7735_WHITE   0xFFFF

void ST7735_Init(void);
void ST7735_DrawPixel(uint8_t x, uint8_t y, uint16_t color);
void ST7735_FillScreen(uint16_t color);
void ST7735_DrawChar(uint8_t x, uint8_t y, char c, uint16_t color);
void ST7735_DrawString(uint8_t x, uint8_t y, const char *str, uint16_t color);
void ST7735_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);

#endif