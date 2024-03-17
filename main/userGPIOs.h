#include <stdbool.h>
#ifndef MAIN_GPIO_APP_H
#define MAIN_GPIO_APP_H

// output buz 19 +
// input read flame sensor
#define GPIO_APP_PIN_BUZ 19
#define GPIO_APP_PIN_FLAME 39
#define GPIO_APP_PIN_RESET_BUTTON 0            //user boot btn
#define GPIO_APP_PIN_BTN    34
// Default interrupt flag
#define ESP_INTR_FLAG_DEFAULT	0




/**
 * Starts gpio_app task
 */
void gpio_app_task_start(void);

void gpio_app_turn_warning(bool state);
#endif