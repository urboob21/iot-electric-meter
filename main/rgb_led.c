/*
 * rgb_led.c
 *
 *  Created on: Nov 19, 2023
 *      Author: Phong Nguyen
 */

#include <stdbool.h>
#include <driver/ledc.h>
#include "rgb_led.h"

// RGB LEDs Configurations Array
ledc_info_t ledc_channel[RGB_CHANNEL_NUM];

// Check
bool g_rgb_led_pwm_init = false;

/**
 * Initializes the setting per channel
 * GPIO
 * MODE
 * TIMER CONFIG
 */
static void rgb_led_pwm_init()
{
	// output 1 - red pin
	ledc_channel[0].channel = LEDC_CHANNEL_0;
	ledc_channel[0].gpio = RGB_GPIO_LED_RED;
	ledc_channel[0].mode = LEDC_HIGH_SPEED_MODE;
	ledc_channel[0].timer_index = LEDC_TIMER_0;

	// output 1 - green pin
	ledc_channel[1].channel = LEDC_CHANNEL_1;
	ledc_channel[1].gpio = RGB_GPIO_LED_GREEN;
	ledc_channel[1].mode = LEDC_HIGH_SPEED_MODE;
	ledc_channel[1].timer_index = LEDC_TIMER_0;

	// output 1 - blue pin
	ledc_channel[2].channel = LEDC_CHANNEL_2;
	ledc_channel[2].gpio = RGB_GPIO_LED_BLUE;
	ledc_channel[2].mode = LEDC_HIGH_SPEED_MODE;
	ledc_channel[2].timer_index = LEDC_TIMER_0;

	// Timer Configuration -  timer 0
	ledc_timer_config_t rgb_timer_config = {
		.duty_resolution = LEDC_TIMER_8_BIT, .freq_hz = 100, .speed_mode = LEDC_HIGH_SPEED_MODE, .timer_num = LEDC_TIMER_0};
	ledc_timer_config(&rgb_timer_config);

	// Channel Configuration - 0,1,2
	for (uint8_t rgb_channel = 0; rgb_channel < RGB_CHANNEL_NUM;
		 rgb_channel++)
	{

		ledc_channel_config_t rgb_channel_config = {
			.channel =
				ledc_channel[rgb_channel].channel,
			.duty = 0,
			.hpoint = 0,
			.gpio_num = ledc_channel[rgb_channel].gpio,
			.intr_type =
				LEDC_INTR_DISABLE,
			.speed_mode =
				ledc_channel[rgb_channel].mode,
			.timer_sel =
				ledc_channel[rgb_channel].timer_index,
		};
		ledc_channel_config(&rgb_channel_config);
	}

	// Point that already init
	g_rgb_led_pwm_init = true;
}

/**
 * Sets the color
 */
static void rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{

	// Set duty PWM based on value
	// 0 - 255

	// RED
	ledc_set_duty(ledc_channel[0].mode, ledc_channel[0].channel, red);
	ledc_update_duty(ledc_channel[0].mode, ledc_channel[0].channel);

	// GREEN
	ledc_set_duty(ledc_channel[1].mode, ledc_channel[1].channel, green);
	ledc_update_duty(ledc_channel[1].mode, ledc_channel[1].channel);

	// BLUE
	ledc_set_duty(ledc_channel[2].mode, ledc_channel[2].channel, blue);
	ledc_update_duty(ledc_channel[2].mode, ledc_channel[2].channel);
}

void rgb_led_display(uint8_t r, uint8_t g, uint8_t b)
{
	if (!g_rgb_led_pwm_init)
	{
		rgb_led_pwm_init();
	}

	// Set the specific color
	rgb_led_set_color(r, g, b);
}

void rgb_led_http_server_started(void)
{
	rgb_led_display(204, 255, 51);
}

void rgb_led_wifi_app_started(void)
{
	rgb_led_display(RGB_COLOR_YELLOW);
}

void rgb_led_wifi_connected(void)
{
	rgb_led_display(0, 255, 153);
}

void rgb_led_mqtt_connected(void)
{
	rgb_led_display(RGB_COLOR_GREEN);
}
