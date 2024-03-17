#ifndef MAIN_LCD2004_APP_H_
#define MAIN_LCD2004_APP_H_
#include "freertos/FreeRTOS.h"
#define LCD2004_I2C_MASTER_SDA_IO 21
#define LCD2004_I2C_MASTER_SCL_IO 22
#define LCD2004_I2C_MASTER_FREQ_HZ 100000
#define LCD2004_I2C_MASTER_NUM 0
#define LCD2004_LCD_ADDR 0x27  // 
#define LCD2004_LCD_COLUMNS 20
#define LCD2004_LCD_ROWS 4
#define LCD2004_LCD_BACKLIGHT 1

void lcd2004_app_start();

/**
 * Define msgId for message
*/
typedef enum
{
    LCD2004_MSG_DISPLAY_HOME =0,
    LCD2004_MSG_DISPLAY_TEMHUM ,
    LCD2004_MSG_DISPLAY_CO ,
    LCD2004_MSG_ON_WARNING,
    LCD2004_MSG_OFF_WARNING
} lcd2004_app_id_message_t;

/**
 * Struct for message queue
 * Expand this based on application
 */
typedef struct
{
	lcd2004_app_id_message_t msgId;
} lcd2004_app_message_t;

/**
 * Sends message function
 */
BaseType_t lcd2004_app_send_message(lcd2004_app_id_message_t msg);

#endif