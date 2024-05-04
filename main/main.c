#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "include.h"

// Register the callback function when wifi connected
void wifi_app_register_connected_events()
{
	mqtt_app_start();
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

	// Reset button
	gpio_app_task_start();

	// Register the funtion callback MQTT when connected successfully wifi host
	wifi_app_set_callback(*wifi_app_register_connected_events);
	
	// Start Wifi
	wifi_app_start();
}