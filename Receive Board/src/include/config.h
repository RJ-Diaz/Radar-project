#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>

// === UART Configuration for HC-12 ===
#define UART_BAUDRATE 9600
#define F_CPU         16000000UL  // ATmega328P default clock

// === Display Pin Configuration ===
#define TFT_CS_PORT    PORTB
#define TFT_CS_DDR     DDRB
#define TFT_CS_PIN     PB2

#define TFT_DC_PORT    PORTB
#define TFT_DC_DDR     DDRB
#define TFT_DC_PIN     PB1

#define TFT_RST_PORT   PORTB
#define TFT_RST_DDR    DDRB
#define TFT_RST_PIN    PB0

// === SPI Configuration ===
#define SPI_MOSI_PIN   PB3
#define SPI_SCK_PIN    PB5

// === Radar Display Settings ===
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  160
#define RADAR_CENTER_X  (DISPLAY_WIDTH / 2)
#define RADAR_CENTER_Y  (DISPLAY_HEIGHT - 1)
#define RADAR_RADIUS    60

#endif