/*
 * rgb_led.h
 *
 *  Created on: Nov 19, 2023
 *      Author: Phong Nguyen
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_
// RGB GPIOs LED
#define RGB_GPIO_LED_RED		23
#define RGB_GPIO_LED_GREEN		22
#define RGB_GPIO_LED_BLUE		21

// Colors
#define RGB_COLOR_RED       255, 0, 0      // Red: R=255, G=0, B=0
#define RGB_COLOR_ORANGE    255, 127, 0    // Orange: R=255, G=127, B=0
#define RGB_COLOR_YELLOW    255, 255, 0    // Yellow: R=255, G=255, B=0
#define RGB_COLOR_GREEN     0, 255, 0      // Green: R=0, G=255, B=0
#define RGB_COLOR_BLUE      0, 0, 255      // Blue: R=0, G=0, B=255
#define RGB_COLOR_INDIGO    46, 43, 95     // Indigo: R=46, G=43, B=95
#define RGB_COLOR_VIOLET    139, 0, 255    // Violet: R=139, G=0, B=255

// RGB LED Channels
#define RGB_CHANNEL_NUM			3

// RGB LED configuration
typedef struct {
	int channel;
	int gpio;
	int mode;
	int timer_index;
} ledc_info_t;

/**
 * User R-G-B color
 */
void rgb_led_display(uint8_t r,uint8_t g,uint8_t b);

/**
 * Color to indicate WiFi application has started.
 */
void rgb_led_wifi_app_started(void);

/**
 * Color to indicate HTTP server has started.
 */
void rgb_led_http_server_started(void);

/**
 * Color to indicate that the ESP32 is connected to an access point.
 */
void rgb_led_wifi_connected(void);

/**
 * Color indicate mqtt connected
*/
void rgb_led_mqtt_connected(void);

#endif /* MAIN_RGB_LED_H_ */
