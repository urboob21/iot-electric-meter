// lcd_functions.h
#ifndef LCD_FUNCTIONS_H
#define LCD_FUNCTIONS_H
#include "lcd.h"
#ifdef __cplusplus
extern "C" {
#endif
#define LCD2004_I2C_MASTER_SDA_IO 21
#define LCD2004_I2C_MASTER_SCL_IO 22
#define LCD2004_I2C_MASTER_FREQ_HZ 100000
#define LCD2004_I2C_MASTER_NUM 0
#define LCD2004_LCD_ADDR 0x27  // 
#define LCD2004_LCD_COLUMNS 20
#define LCD2004_LCD_ROWS 4
#define LCD2004_LCD_BACKLIGHT 1

void initialise();
void lcd_demo();

extern lcd_handle_t lcd_handle;
#ifdef __cplusplus
}
#endif

#endif // LCD_FUNCTIONS_H