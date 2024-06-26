#include "lcd2004.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"

static const char *TAG = "LCD_2004";

lcd_handle_t lcd_handle = LCD_HANDLE_DEFAULT_CONFIG();

void clear_row(int row)
{
    lcd_set_cursor(&lcd_handle, 0, row);
    lcd_write_str(&lcd_handle, "                   ");
}

// Perform initilisation functions
void initialise()
{
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = LCD2004_I2C_MASTER_SDA_IO,
        .scl_io_num = LCD2004_I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = LCD2004_I2C_MASTER_FREQ_HZ,
    };

    // Initialise i2c
    ESP_LOGD(TAG, "Installing i2c driver in master mode on channel %d", I2C_MASTER_NUM);
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0));
    ESP_LOGD(TAG,
             "Configuring i2c parameters.\n\tMode: %d\n\tSDA pin:%d\n\tSCL pin:%d\n\tSDA pullup:%d\n\tSCL pullup:%d\n\tClock speed:%.3fkHz",
             i2c_config.mode, i2c_config.sda_io_num, i2c_config.scl_io_num,
             i2c_config.sda_pullup_en, i2c_config.scl_pullup_en,
             i2c_config.master.clk_speed / 1000.0);
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &i2c_config));

    // Modify default lcd_handle details
    lcd_handle.i2c_port = LCD2004_I2C_MASTER_NUM;
    lcd_handle.address = LCD2004_LCD_ADDR;
    lcd_handle.columns = LCD2004_LCD_COLUMNS;
    lcd_handle.rows = LCD2004_LCD_ROWS;
    lcd_handle.backlight = LCD2004_LCD_BACKLIGHT;
    lcd_handle.display_mode = LCD_ENTRY_DISPLAY_NO_SHIFT;

    // Initialise LCD
    ESP_ERROR_CHECK(lcd_init(&lcd_handle));

    return;
}

/**
 * @brief Demonstrate the LCD
 */
void lcd_demo()
{
    char num[20];
    char c = '!'; // first ascii char
    char str[80];
    bool lr_test_done = false;

    ESP_ERROR_CHECK(lcd_probe(&lcd_handle));
    ESP_LOGI(TAG, "Clear screen");
    lcd_clear_screen(&lcd_handle);
    ESP_LOGI(TAG, "Write string:<columns>x<rows> I2C LCD");
    sprintf(str, "%dx%d I2C LCD", lcd_handle.columns, lcd_handle.rows);
    lcd_write_str(&lcd_handle, str);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Clear screen");
    lcd_clear_screen(&lcd_handle);
    ESP_LOGI(TAG, "Write string:Lets write some characters!");
    lcd_write_str(&lcd_handle, "Lets write some characters!");
    lcd_backlight(&lcd_handle);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Clear screen");
    lcd_clear_screen(&lcd_handle);
    lcd_blink(&lcd_handle);
    lcd_cursor(&lcd_handle);
    for (int row = 0; row < LCD_ROWS; row++)
    {
        ESP_LOGI(TAG, "Set cursor on column 0, row %d", row);
        lcd_set_cursor(&lcd_handle, 0, row);
        c = '!';
        while (lcd_handle.cursor_row == row)
        {
            sprintf(num, "%c", c);
            lcd_write_char(&lcd_handle, c);
            c++;

            // Test right to left
            if (lcd_handle.cursor_column == (lcd_handle.columns / 2) && (!lr_test_done))
            {
                ESP_LOGI(TAG, "Testing text direction right to left");
                lcd_right_to_left(&lcd_handle);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                ESP_LOGD(TAG,
                         "LCD handle:\n\ti2c_port: %d\n\tAddress: 0x%0x\n\tColumns: %d\n\tRows: %d\n\tDisplay Function: 0x%0x\n\tDisplay Control: 0x%0x\n\tDisplay Mode: 0x%0x\n\tCursor Column: %d\n\tCursor Row: %d\n\tBacklight: %d\n\tInitialised: %d",
                         lcd_handle.i2c_port, lcd_handle.address, lcd_handle.columns, lcd_handle.rows,
                         lcd_handle.display_function, lcd_handle.display_control, lcd_handle.display_mode,
                         lcd_handle.cursor_column, lcd_handle.cursor_row, lcd_handle.backlight,
                         lcd_handle.initialized);
                while (lcd_handle.cursor_column > 0)
                {
                    sprintf(num, "%c", c);
                    lcd_write_char(&lcd_handle, c);
                    c++;
                }
                ESP_LOGI(TAG, "Reverting text direction to left to right");
                lcd_left_to_right(&lcd_handle);
                lr_test_done = true;
                ESP_LOGD(TAG,
                         "LCD handle:\n\ti2c_port: %d\n\tAddress: 0x%0x\n\tColumns: %d\n\tRows: %d\n\tDisplay Function: 0x%0x\n\tDisplay Control: 0x%0x\n\tDisplay Mode: 0x%0x\n\tCursor Column: %d\n\tCursor Row: %d\n\tBacklight: %d\n\tInitialised: %d",
                         lcd_handle.i2c_port, lcd_handle.address, lcd_handle.columns, lcd_handle.rows,
                         lcd_handle.display_function, lcd_handle.display_control, lcd_handle.display_mode,
                         lcd_handle.cursor_column, lcd_handle.cursor_row, lcd_handle.backlight,
                         lcd_handle.initialized);
            }
        }
        lr_test_done = false;
        ESP_LOGD(TAG,
                 "LCD handle:\n\ti2c_port: %d\n\tAddress: 0x%0x\n\tColumns: %d\n\tRows: %d\n\tDisplay Function: 0x%0x\n\tDisplay Control: 0x%0x\n\tDisplay Mode: 0x%0x\n\tCursor Column: %d\n\tCursor Row: %d\n\tBacklight: %d\n\tInitialised: %d",
                 lcd_handle.i2c_port, lcd_handle.address, lcd_handle.columns, lcd_handle.rows,
                 lcd_handle.display_function, lcd_handle.display_control, lcd_handle.display_mode,
                 lcd_handle.cursor_column, lcd_handle.cursor_row, lcd_handle.backlight,
                 lcd_handle.initialized);
        if (row % 2)
        {
            ESP_LOGD(TAG,
                     "LCD handle:\n\ti2c_port: %d\n\tAddress: 0x%0x\n\tColumns: %d\n\tRows: %d\n\tDisplay Function: 0x%0x\n\tDisplay Control: 0x%0x\n\tDisplay Mode: 0x%0x\n\tCursor Column: %d\n\tCursor Row: %d\n\tBacklight: %d\n\tInitialised: %d",
                     lcd_handle.i2c_port, lcd_handle.address, lcd_handle.columns, lcd_handle.rows,
                     lcd_handle.display_function, lcd_handle.display_control, lcd_handle.display_mode,
                     lcd_handle.cursor_column, lcd_handle.cursor_row, lcd_handle.backlight,
                     lcd_handle.initialized);
            ESP_LOGI(TAG, "Shift display left");
            lcd_display_shift_left(&lcd_handle);
            ESP_LOGD(TAG,
                     "LCD handle:\n\ti2c_port: %d\n\tAddress: 0x%0x\n\tColumns: %d\n\tRows: %d\n\tDisplay Function: 0x%0x\n\tDisplay Control: 0x%0x\n\tDisplay Mode: 0x%0x\n\tCursor Column: %d\n\tCursor Row: %d\n\tBacklight: %d\n\tInitialised: %d",
                     lcd_handle.i2c_port, lcd_handle.address, lcd_handle.columns, lcd_handle.rows,
                     lcd_handle.display_function, lcd_handle.display_control, lcd_handle.display_mode,
                     lcd_handle.cursor_column, lcd_handle.cursor_row, lcd_handle.backlight,
                     lcd_handle.initialized);
        }
        else
        {
            ESP_LOGD(TAG,
                     "LCD handle:\n\ti2c_port: %d\n\tAddress: 0x%0x\n\tColumns: %d\n\tRows: %d\n\tDisplay Function: 0x%0x\n\tDisplay Control: 0x%0x\n\tDisplay Mode: 0x%0x\n\tCursor Column: %d\n\tCursor Row: %d\n\tBacklight: %d\n\tInitialised: %d",
                     lcd_handle.i2c_port, lcd_handle.address, lcd_handle.columns, lcd_handle.rows,
                     lcd_handle.display_function, lcd_handle.display_control, lcd_handle.display_mode,
                     lcd_handle.cursor_column, lcd_handle.cursor_row, lcd_handle.backlight,
                     lcd_handle.initialized);
            ESP_LOGI(TAG, "Shift display right");
            lcd_display_shift_right(&lcd_handle);
            ESP_LOGD(TAG,
                     "LCD handle:\n\ti2c_port: %d\n\tAddress: 0x%0x\n\tColumns: %d\n\tRows: %d\n\tDisplay Function: 0x%0x\n\tDisplay Control: 0x%0x\n\tDisplay Mode: 0x%0x\n\tCursor Column: %d\n\tCursor Row: %d\n\tBacklight: %d\n\tInitialised: %d",
                     lcd_handle.i2c_port, lcd_handle.address, lcd_handle.columns, lcd_handle.rows,
                     lcd_handle.display_function, lcd_handle.display_control, lcd_handle.display_mode,
                     lcd_handle.cursor_column, lcd_handle.cursor_row, lcd_handle.backlight,
                     lcd_handle.initialized);
        }
        ESP_LOGI(TAG, "Finished row %d", row);
        if (LOG_LOCAL_LEVEL > ESP_LOG_INFO)
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        else
            vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    lcd_no_backlight(&lcd_handle);
    ESP_LOGI(TAG, "LCD Demo finished");
}
