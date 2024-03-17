/*
 * http_server.h
 *
 *  Created on: Nov 19, 2023
 *      Author: Phong Nguyen
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

#define OTA_UPDATE_PENDING 0
#define OTA_UPDATE_SUCCESSFUL 1
#define OTA_UPDATE_FAILED -1

/**
 * Connection status for Wifi
 */
typedef enum http_server_wifi_connect_status
{
	NONE = 0,
	HTTP_WIFI_STATUS_CONNECTING,
	HTTP_WIFI_STATUS_CONNECT_FAILED,
	HTTP_WIFI_STATUS_CONNECT_SUCCESS,
	HTTP_WIFI_STATUS_DISCONNECTED
} http_server_wifi_connect_status_e;

/**
 * Message for the HTTP Server
 */
typedef enum
{
	HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL,
	HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
	HTTP_MSG_OTA_UPDATE_FAILED,
	HTTP_MSG_OTA_UPATE_INITIALIZED,
	HTTP_MSG_WIFI_USER_DISCONNECT
} http_server_message_id_e;

/**
 * Structure for the message queue
 */
typedef struct
{
	http_server_message_id_e msgId;
} http_server_queue_message_t;

BaseType_t http_server_monitor_send_message(http_server_message_id_e msgId);

/**
 * Start the HTTP Server
 */
void http_server_start();

/**
 * Stop the HTTP Server
 */
void http_server_stop();

#endif /* MAIN_HTTP_SERVER_H_ */
