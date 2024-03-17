/*
 * http_server.c
 *
 *  Created on: Nov 19, 2023
 *      Author: Phong Nguyen
 */

#include "sys/param.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_http_server.h"

#include "freertos/queue.h"

#include "esp_ota_ops.h"

#include "http_server.h"
#include "tasks_common.h"
#include "dht22.h"
#include "wifi_app.h"
#include "esp_timer.h"

static const char TAG[] = "HTTP Server application";

// Wifi connect status
static int g_wifi_connect_status = NONE;

/**
 * Timer handler
 */
void http_server_fw_update_reset_callback(void *arg)
{
	ESP_LOGI(TAG,
			 "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}

// ESP32 timer configuration passed to esp_timer_create
const esp_timer_create_args_t fw_update_reset_args = {.callback =
														  &http_server_fw_update_reset_callback,
													  .arg = NULL,
													  .dispatch_method =
														  ESP_TIMER_TASK,
													  .name = "fw_update_reset"};

// Instance 1 timer
esp_timer_handle_t fw_update_reset_timer;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

// HTTP Server task handle freeRTOS
static TaskHandle_t tHandler_http_server_monitor = NULL;

// HTTP Server task handle - instance of HTTP Server
static httpd_handle_t http_server_handle = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle = NULL;

// Embedded file - define the start - end Address
extern const uint8_t jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");

extern const uint8_t app_css_start[] asm("_binary_app_css_start");
extern const uint8_t app_css_end[] asm("_binary_app_css_end");

extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

/**
 * HTTP Sever monitor send message
 */
BaseType_t http_server_monitor_send_message(http_server_message_id_e msgId)
{
	http_server_queue_message_t msg;
	msg.msgId = msgId;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

/**
 * Checks the g_fw_update_status and creates the fw_update_reset timer if g_fw_update_status is true.
 */
static void http_server_fw_update_reset_timer()
{
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		ESP_LOGI(TAG,
				 "http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the TIMER
		ESP_ERROR_CHECK(
			esp_timer_create(&fw_update_reset_args,
							 &fw_update_reset_timer));

		// Start on-shot timer - timeout_us
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset_timer, 8000000));
	}
	else
	{
		ESP_LOGI(TAG,
				 "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}

/**
 * HTTP server monitor handle used to track events of the HTTP Server
 */
static void http_server_monitor()
{
	http_server_queue_message_t msg;

	for (;;)
	{
		if (xQueueReceive(http_server_monitor_queue_handle, &msg,
						  portMAX_DELAY))
		{
			ESP_LOGI(TAG, "Received the MESSAGE !");
			switch (msg.msgId)
			{
			case HTTP_MSG_WIFI_CONNECT_INIT:
				ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");

				g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;

				break;

			case HTTP_MSG_WIFI_CONNECT_SUCCESS:
				ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");

				g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;

				break;

			case HTTP_MSG_WIFI_CONNECT_FAIL:
				ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");

				g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;
				break;

			case HTTP_MSG_WIFI_USER_DISCONNECT:
				ESP_LOGI(TAG, "HTTP_MSG_WIFI_USER_DISCONNECT");

				g_wifi_connect_status = HTTP_WIFI_STATUS_DISCONNECTED;

				break;

			case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
				ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
				g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
				http_server_fw_update_reset_timer();
				break;

			case HTTP_MSG_OTA_UPDATE_FAILED:
				ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
				g_fw_update_status = OTA_UPDATE_FAILED;
				break;

			default:
				break;
			}
		}
	}
}

/**
 * JQuery get handler is requested when accessing the web page
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "JQuery requested");

	httpd_resp_set_type(r, "application/javascript");
	httpd_resp_send(r, (const char *)jquery_3_3_1_min_js_start,
					jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

/**
 * Send index.html Webpage
 */
static esp_err_t http_server_html_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "HTML requested");

	httpd_resp_set_type(r, "text/html");
	httpd_resp_send(r, (const char *)index_html_start,
					index_html_end - index_html_start);

	return ESP_OK;
}

/**
 * app.css get handler is requested when accessing the web page
 */
static esp_err_t http_server_css_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "CSS requested");

	httpd_resp_set_type(r, "text/css");
	httpd_resp_send(r, (const char *)app_css_start,
					app_css_end - app_css_start);

	return ESP_OK;
}

/**
 * app.js get handler is requested when accessing the web page
 */
static esp_err_t http_server_js_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "JS requested");

	httpd_resp_set_type(r, "appication/javascript");
	httpd_resp_send(r, (const char *)app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}

/**
 * repspond icon web page
*/
static esp_err_t http_server_ico_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "ICO requested");

	httpd_resp_set_type(r, "image/x-icon");
	httpd_resp_send(r, (const char *)favicon_ico_start,
					favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

/**
 * Receives the .bin file fia the web page and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be started.
 */
static esp_err_t http_server_ota_update_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "OTA Update requested");

	esp_ota_handle_t ota_handle;
	char ota_buff[1024];
	int content_received = 0;
	int content_length = r->content_len;
	int recv_data_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(
		NULL);
	// Read the data for the request
	do
	{
		// Number of bytes read into the buffer successfully
		recv_data_len = httpd_req_recv(r, ota_buff,
									   MIN(sizeof(ota_buff), content_length));
		if (recv_data_len < 0)
		{

			// Check timeout
			if (recv_data_len == HTTPD_SOCK_ERR_TIMEOUT)
			{
				ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
				continue; ///> Retry receiving if timeout occurred
			}

			// Another case
			ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other Error %d",
					 recv_data_len);
			return ESP_FAIL;
		}

		printf("http_server_OTA_update_handler: OTA RX: %d of %d\r",
			   content_received, content_length);

		// Header data
		if (!is_req_body_started)
		{

			// Already
			is_req_body_started = true;

			// Get the location of the .bin file - start address
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
			int body_part_len = recv_data_len - (body_start_p - ota_buff);

			printf("http_server_OTA_update_handler: OTA file size: %d\r\n",
				   content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN,
										  &ota_handle);
			if (err != ESP_OK)
			{
				printf(
					"http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf(
					"http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%lx\r\n",
					update_partition->subtype, update_partition->address);
			}

			// Write this first part of the data
			esp_ota_write(ota_handle, body_start_p, body_part_len);
			content_received += body_part_len;
		}
		else
		{

			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_data_len);
			content_received += recv_data_len;
		}

	} while (recv_data_len > 0 && content_received < content_length);

	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition =
				esp_ota_get_boot_partition();
			ESP_LOGI(TAG,
					 "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%lx",
					 boot_partition->subtype, boot_partition->address);
			flash_successful = true;
		}
		else
		{
			ESP_LOGI(TAG, "http_server_ota_update_handler: FLASHED ERROR!!!");
		}
	}
	else
	{
		ESP_LOGI(TAG, "http_server_ota_update_handler: esp_ota_end ERROR!!!");
	}

	// We won't update the global variables throughout the file, so send the message about the status
	if (flash_successful)
	{
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL);
	}
	else
	{
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
	}

	return ESP_OK;
}

/**
 * OTA status handler responds with the firmware update status after the OTA update is started
 * and responds with the compile time/date when the page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
esp_err_t http_server_ota_status_handler(httpd_req_t *r)
{
	char otaJSON[100];

	ESP_LOGI(TAG, "OTAstatus requested");

	sprintf(otaJSON,
			"{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}",
			g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(r, "application/json");
	httpd_resp_send(r, otaJSON, strlen(otaJSON));

	return ESP_OK;
}

/**
 * DHT sensor readings JSON handler responds with DHT22 sensor data
 */
esp_err_t http_server_dht_sensor_json_handler(httpd_req_t *r)
{

	ESP_LOGI(TAG, "/dhtSensor.json requested");

	char dhtSensorJSON[100];
	sprintf(dhtSensorJSON, "{\"temp\":\"%.1f\",\"humidity\":\"%.1f\"}",
			getTemperature(), getHumidity());
	httpd_resp_set_type(r, "application/json");
	httpd_resp_send(r, dhtSensorJSON, strlen(dhtSensorJSON));
	return ESP_OK;
}

/**
 * wifiConnect.json handler is invoked after the connect button is pressed
 * and handles receiving the SSID and password entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "/wifiConnect.json requested");
	size_t len_ssid = 0, len_pass = 0;
	char *ssid_str = NULL, *pass_str = NULL;

	// Get SSID header
	len_ssid = httpd_req_get_hdr_value_len(r, "my-connect-ssid") + 1;
	if (len_ssid > 1)
	{
		ssid_str = malloc(len_ssid);
		if (httpd_req_get_hdr_value_str(r, "my-connect-ssid", ssid_str,
										len_ssid) == ESP_OK)
		{
			ESP_LOGI(TAG,
					 "http_server_wifi_connect_json_handler: Found header => my-connect-ssid: %s",
					 ssid_str);
		}
	}
	// Get Password header
	len_pass = httpd_req_get_hdr_value_len(r, "my-connect-pwd") + 1;
	if (len_pass > 1)
	{
		pass_str = malloc(len_pass);
		if (httpd_req_get_hdr_value_str(r, "my-connect-pwd", pass_str, len_pass) == ESP_OK)
		{
			ESP_LOGI(TAG,
					 "http_server_wifi_connect_json_handler: Found header => my-connect-pwd: %s",
					 pass_str);
		}
	}

	// Update the Wifi networks configuration and let the wifi application know
	wifi_config_t *wifi_config = wifi_app_get_wifi_config();
	memset(wifi_config, 0x00, sizeof(wifi_config_t));
	memcpy(wifi_config->sta.ssid, ssid_str, len_ssid);
	memcpy(wifi_config->sta.password, pass_str, len_pass);
	wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);
	free(ssid_str);
	free(pass_str);
	return ESP_OK;
}

/**
 * wifiDisconnect.json handler is invoked after the disconnect button is pressed
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_disconnect_json_handler(httpd_req_t *r)
{
	ESP_LOGI(TAG, "wifiDisconect.json requested");

	wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);

	return ESP_OK;
}
/**
 * wifiConnectStatus handler updates the connection status for the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnectStatus requested");

	char statusJSON[100];

	sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, statusJSON, strlen(statusJSON));

	return ESP_OK;
}

/**
 * wifiConnectInfo.json handler updates the web page with connection information.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_get_wifi_connect_info_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnectInfo.json requested");

	char ipInfoJSON[200];
	memset(ipInfoJSON, 0, sizeof(ipInfoJSON));

	char ip[IP4ADDR_STRLEN_MAX];
	char netmask[IP4ADDR_STRLEN_MAX];
	char gw[IP4ADDR_STRLEN_MAX];

	if (g_wifi_connect_status == HTTP_WIFI_STATUS_CONNECT_SUCCESS)
	{
		wifi_ap_record_t wifi_data;
		ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_data));
		char *ssid = (char *)wifi_data.ssid;

		esp_netif_ip_info_t ip_info;
		ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_sta, &ip_info));
		esp_ip4addr_ntoa(&ip_info.ip, ip, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.netmask, netmask, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.gw, gw, IP4ADDR_STRLEN_MAX);

		sprintf(ipInfoJSON, "{\"ip\":\"%s\",\"netmask\":\"%s\",\"gw\":\"%s\",\"ap\":\"%s\"}", ip, netmask, gw, ssid);
	}

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ipInfoJSON, strlen(ipInfoJSON));

	return ESP_OK;
}

/**
 * Sets up the default httpd server configuration.
 * @return http server instance handle if successful, NULL otherwise.
 */
static httpd_handle_t http_server_configure()
{
	// Default configuration
	httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
	http_config.core_id = HTTP_SERVER_TASK_CORE_ID;
	http_config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
	http_config.task_priority = HTTP_SERVER_TASK_PRIORITY;
	// Increase uri handlers
	http_config.max_uri_handlers = 20;
	// Increase the timeout limits
	http_config.recv_wait_timeout = 10;
	http_config.send_wait_timeout = 10;

	ESP_LOGI(TAG,
			 "http_server_configure: Starting server on port: '%d' with task priority: '%d'",
			 http_config.server_port, http_config.task_priority);

	// Create HTTP Server Monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "HTTP_SERVER_MONITOR",
							HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY,
							&tHandler_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);
	// Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3,
													sizeof(http_server_queue_message_t));

	// Start the HTTP server
	// If Instance created successfully
	if (httpd_start(&http_server_handle, &http_config) == ESP_OK)
	{

		ESP_LOGI(TAG, "http_server_configure: Registering URI handlers");
		// Register HTTP Server x URIs
		// Register query handler
		httpd_uri_t jquery_js = {.uri = "/jquery-3.3.1.min.js", .method = HTTP_GET, .handler = http_server_jquery_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		// register index.html handler
		httpd_uri_t index_html = {.uri = "/", .method = HTTP_GET, .handler = http_server_html_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &index_html);

		// register app.css handler
		httpd_uri_t app_css = {.uri = "/app.css", .method = HTTP_GET, .handler = http_server_css_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &app_css);

		// register app.js handler
		httpd_uri_t app_js = {.uri = "/app.js", .method = HTTP_GET, .handler = http_server_js_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &app_js);

		// register favicon.ico handler
		httpd_uri_t favicon_ico = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = http_server_ico_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);

		// register OTA Update handler
		httpd_uri_t ota_update = {.uri = "/OTAupdate", .method = HTTP_POST, .handler = http_server_ota_update_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &ota_update);

		// register OTAstatus handler
		httpd_uri_t OTA_status = {.uri = "/OTAstatus", .method = HTTP_POST, .handler = http_server_ota_status_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &OTA_status);

		// register dhtSensor.json handler
		httpd_uri_t dht_sensor_json = {.uri = "/dhtSensor.json", .method = HTTP_GET, .handler = http_server_dht_sensor_json_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &dht_sensor_json);

		// register wifiConnect.json handler
		httpd_uri_t wifi_connect_json = {.uri = "/wifiConnect.json", .method = HTTP_POST, .handler = http_server_wifi_connect_json_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_json);

		httpd_uri_t wifi_disconnect_json = {.uri = "/wifiDisconnect.json", .method = HTTP_DELETE, .handler = http_server_wifi_disconnect_json_handler, .user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &wifi_disconnect_json);

		// register wifiConnectStatus.json handler
		httpd_uri_t wifi_connect_status_json = {.uri = "/wifiConnectStatus",
												.method = HTTP_POST,
												.handler =
													http_server_wifi_connect_status_json_handler,
												.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle,
								   &wifi_connect_status_json);

		// register wifiConnectInfo.json handler
		httpd_uri_t wifi_connect_info_json = {
			.uri = "/wifiConnectInfo.json",
			.method = HTTP_GET,
			.handler = http_server_get_wifi_connect_info_json_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_info_json);

		return http_server_handle;
	}

	return NULL;
}

/**
 * Start the HTTP Server
 */
void http_server_start()
{
	if (http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
		ESP_LOGI(TAG, "HTTP SERVER CREATED SUCCESSFULLY !");
	}
}

/**
 * Stop the HTTP Server
 */
void http_server_stop()
{
	if (http_server_handle != NULL)
	{
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG, "STOPPING HTTP SERVER");
		http_server_handle = NULL;
	}
	if (tHandler_http_server_monitor)
	{
		vTaskDelete(tHandler_http_server_monitor);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server monitor");
		tHandler_http_server_monitor = NULL;
	}
}
