#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "userGPIOs.h"
#include "tasksCommon.h"
#include "wifiApplication.h"
#include "httpServer.h"
#include "nvs.h"
#include "pzem.h"
#include "time/sntp_time_sync.h"
#include "lcd2004.h"
SemaphoreHandle_t xSemaphoreAWS;
pzem_sensor_t pzemData;

int aws_iot_demo_main(int argc, char **argv);

// Register the callback function when wifi connected
void wifi_app_register_connected_events()
{
	// settup the sntp
	sntp_time_sync_task_start();
	// start mqtt
	aws_iot_demo_main(0, NULL);
}

void pzem_lcd_display()
{

	char str[80];
	// display to the LCD
	lcd_clear_screen(&lcd_handle);
	lcd_set_cursor(&lcd_handle, 0, 0);
	lcd_write_str(&lcd_handle, "D.AP: ");
	printf(str, "%1.f", pzemData.voltage);
	lcd_write_str(&lcd_handle, str);
	lcd_write_str(&lcd_handle, "V ");

	lcd_set_cursor(&lcd_handle, 0, 1);
	lcd_write_str(&lcd_handle, "DONG: ");
	printf(str, "%2.f", pzemData.current);
	lcd_write_str(&lcd_handle, str);
	lcd_write_str(&lcd_handle, "A ");

	lcd_set_cursor(&lcd_handle, 0, 2);
	lcd_write_str(&lcd_handle, "C.S: ");
	printf(str, "%1.f", pzemData.power);
	lcd_write_str(&lcd_handle, str);
	lcd_write_str(&lcd_handle, "W ");

	lcd_set_cursor(&lcd_handle, 0, 3);
	lcd_write_str(&lcd_handle, "SO CHI: ");
	printf(str, "%3.f", pzemData.energy);
	lcd_write_str(&lcd_handle, str);
	lcd_write_str(&lcd_handle, "kWh ");
}
//++++++++++++++++++++++++++++++++++++++++++++++++//
//					ENTRY POINT					  //
//++++++++++++++++++++++++++++++++++++++++++++++++//
void app_main(void)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	xSemaphoreAWS = xSemaphoreCreateMutex();

	// init the LCD
	// initialise();

	// pzem_sensor_init();

	// Register the funtion callback MQTT when connected successfully wifi host
	wifi_app_set_callback(*wifi_app_register_connected_events);

	// Start WIFI application
	wifi_app_start();

	while (1)
	{
		// get and display data from pzem
		// pzem_sensor_request(&pzemData, PZEM_DEFAULT_ADDR);
		// pzem_lcd_display();
		xSemaphoreTake(xSemaphoreAWS, portMAX_DELAY);
		pzemData.current=100;
		pzemData.voltage=200;
		pzemData.frequency=60;
		pzemData.energy=110;
		pzemData.energy=0.001;
		// send msg to mqtt task to server
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		xSemaphoreGive(xSemaphoreAWS);


	}
}