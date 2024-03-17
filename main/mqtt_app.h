#ifndef MAIN_MQTT_AP_H
#define MAIN_MQTT_AP_H

#define MQTT_APP_BROKER_URI "mqtts://660f28e90d254c3a969676062a264527.s2.eu.hivemq.cloud:8883"
#define MQTT_APP_USER "nphong2103"
#define MQTT_APP_PASSWORD "210301Phong"
#define MQTT_APP_TOPIC_SUB "topic/request"
#define MQTT_APP_TOPIC_SUB_CO "topic/sub_co"
#define MQTT_APP_TOPIC_PUB "topic/reply"
#define MQTT_APP_TOPIC_SUB_BUZZ "topic/request_buzz"
#define MQTT_APP_TOPIC_PUB_TEMP "topic/push_temp"
#define MQTT_APP_TOPIC_PUB_HUMI "topic/push_humi"
#define MQTT_APP_TOPIC_PUB_CO "topic/sub_co"

/**
 * Connection status for MQTT
 */
typedef enum mqtt_app_connect_status
{
	MQTT_APP_CONNECT_NONE = 0,
	MQTT_APP_CONNECT_SUCCESS
} mqtt_app_connect_status_e;

/**
 * Message IDs for the MQTT application t
 */
typedef enum
{
	MQTT_APP_MSG_START = 0,
	MQTT_APP_MSG_CONNECTED,
	MQTT_APP_MSG_DISCONNECTED,
	MQTT_APP_MSG_RECONNECT,
	MQTT_APP_MSG_SUBSCRIBED,
	MQTT_APP_MSG_PUBLISHED,
	MQTT_APP_MSG_ONWARNING,
	MQTT_APP_MSG_OFFWARNING,

} mqtt_app_message_id_e;

/**
 * Message struct  for the MQTT app
 */
typedef struct
{
	mqtt_app_message_id_e msgId;
	char *msgTopic;
	uint8_t msgLenTopic;
	char *msgData;
	uint8_t msgLenData;
} mqtt_app_message_t;

/**
 * Sends message function
 */
BaseType_t mqtt_app_send_message(mqtt_app_message_id_e msg);

/**
 * Sends message function with topic/data
 */
BaseType_t mqtt_app_send_message_with(mqtt_app_message_id_e msgId, char *topic, uint8_t lenTopic, char *data, uint8_t lenData);

/**
 * Start mqtt client when wifi connected station mode
 */
void mqtt_app_start();

/**
 * Stop mqtt client when wifi disconnected station mode
 */
void mqtt_app_stop();

#endif