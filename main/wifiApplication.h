/*
 * wifi_app.h
 *
 *  Created on: Nov 19, 2023
 *      Author: Phong Nguyen
 */
#include "esp_netif.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

// Callback typedef - define the funtion in pointer
typedef void (*wifi_connected_event_callback_t)(void);

// WiFi application settings
#define WIFI_AP_SSID "THING-DEVICE-0000-ESP32" // AP name
#define WIFI_AP_PASSWORD "password"			   // AP password
#define WIFI_AP_CHANNEL 1					   // AP channel
#define WIFI_AP_SSID_HIDDEN 0				   // AP visibility
#define WIFI_AP_MAX_CONNECTIONS 5			   // AP max clients
#define WIFI_AP_BEACON_INTERVAL 100			   // AP beacon: 100 milliseconds recommended
#define WIFI_AP_IP "192.168.0.1"			   // AP default IP
#define WIFI_AP_GATEWAY "192.168.0.1"		   // AP default Gateway (should be the same as the IP)
#define WIFI_AP_NETMASK "255.255.255.0"		   // AP netmask
#define WIFI_AP_BANDWIDTH WIFI_BW_HT20		   // AP bandwidth 20 MHz (40 MHz is the other option)
#define WIFI_STA_POWER_SAVE WIFI_PS_NONE	   // Power save not used
#define MAX_SSID_LENGTH 32					   // IEEE standard maximum
#define MAX_PASSWORD_LENGTH 64				   // IEEE standard maximum
#define MAX_CONNECTION_RETRIES 3			   // Retry number on disconnect

extern esp_netif_t *esp_netif_sta;

/**
 * Define msgId for message
 */
typedef enum
{
	WIFI_APP_MSG_START_HTTP_SERVER = 0,		  // default start
	WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER, // when press Btn Connect on web-config-local
	WIFI_APP_MSG_STA_CONNECTED_GOT_IP,		  // when connect with station
	WIFI_APP_MSG_STA_DISCONNECTED,
	WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS,	   // when request load
	WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT // when press Btn Disconnect
} wifi_app_message_t;

/**
 * Struct for message queue
 * Expand this based on application
 */
typedef struct
{
	wifi_app_message_t msgId;
} wifi_app_queue_message_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the wifi_app_message_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
 * @note Expand the parameter list based on your requirements e.g. how you've expanded the wifi_app_queue_message_t.
 */
BaseType_t wifi_app_send_message(wifi_app_message_t msgID);
/**
 * Gets the wifi configuration
 */
wifi_config_t *wifi_app_get_wifi_config(void);

/**
 * Sets the callback function.
 */
void wifi_app_set_callback(wifi_connected_event_callback_t cbFuntion);

/**
 * Calls the callback function.
 */
void wifi_app_call_callback(void);

/**
 * Starts the WIFI task
 */
void wifi_app_start();

#endif /* MAIN_WIFI_APP_H_ */
