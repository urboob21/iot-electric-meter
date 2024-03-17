#include "include.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "esp_log.h"
#include "esp_tls.h"
#include "components/rgb/rgb_led.h"

// Tag used for ESP Serial Console Message
static const char TAG[] = "MQTT Application";

int coResult;
// Task Handle
TaskHandle_t tHandler_mqtt_app = NULL;

// Embedded certificate .bem file
extern const uint8_t mqtt_eclipseprojects_io_pem_start[] asm("_binary_mqtt_eclipseprojects_io_pem_start");
extern const uint8_t mqtt_eclipseprojects_io_pem_end[] asm("_binary_mqtt_eclipseprojects_io_pem_end");

// Queue handle msg
static QueueHandle_t mqtt_app_queue_handle;

// Client instance
esp_mqtt_client_handle_t mqtt_client = NULL;

// MQTT connect status
int g_mqtt_connect_status = MQTT_APP_CONNECT_NONE;
/**
 * Sends message function
 */
BaseType_t mqtt_app_send_message(mqtt_app_message_id_e msgId)
{
    mqtt_app_message_t msg;
    msg.msgId = msgId;
    return xQueueSend(mqtt_app_queue_handle, &msg, portMAX_DELAY);
}

char *createString(const char *msgTopic, uint8_t msgLenTopic)
{
    // Allocate memory for the string, including space for the null terminator
    char *myString = malloc(msgLenTopic + 1);

    // Check if memory allocation was successful
    if (myString == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Copy the characters from msgTopic to myString
    strncpy(myString, msgTopic, msgLenTopic);

    // Ensure null termination
    myString[msgLenTopic] = '\0';

    return myString;
}

/**
 * Sends message function with topic/data
 */
BaseType_t mqtt_app_send_message_with(mqtt_app_message_id_e msgId, char *topic, uint8_t lenTopic, char *data, uint8_t lenData)
{
    mqtt_app_message_t msg;
    msg.msgId = msgId;
    msg.msgTopic = topic;
    msg.msgLenTopic = lenTopic;
    msg.msgData = data;
    msg.msgLenData = lenData;
    return xQueueSend(mqtt_app_queue_handle, &msg, portMAX_DELAY);
}

int setValue(char *str)
{
    // Phân tích chuỗi JSON
    cJSON *root = cJSON_Parse(str);

    if (root == NULL)
    {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return 1;
    }

    // Lấy giá trị của "data"
    cJSON *data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data_obj != NULL && cJSON_IsString(data_obj))
    {
        const char *data_value = data_obj->valuestring;

        // Xử lý chuỗi để lấy giá trị số
        int result;
        if (sscanf(data_value, "%d", &result) == 1)
        {
            printf("Kết quả là: %d\n", result);
            coResult = result;
            lcd2004_app_send_message(LCD2004_MSG_DISPLAY_CO);
        }
        else
        {
            printf("Không thể trích xuất giá trị.\n");
        }
    }
    else
    {
        printf("Không tìm thấy khóa 'data' hoặc giá trị không phải là chuỗi.\n");
    }

    // Giải phóng bộ nhớ
    cJSON_Delete(root);
    return 0;
}
/**
 * Proccess received mqtt data from specific topic
 */
static void mqtt_app_proccess_received_data(char *pTopicBuff, uint8_t lenTopic, char *pDataBuff, uint8_t lenData)
{
    /**
     * tocpic: device_buzzer
     * data: [cmd] ON/OF
     *
     */
    // The result like String

    char *strTopic = createString(pTopicBuff, lenTopic);

    char *strData = createString(pDataBuff, lenData);

    printf("proccess: %s : %s \n", strTopic, strData);
    if (strcmp(strTopic, MQTT_APP_TOPIC_SUB_BUZZ) == 0)
    {
        bool state = false;
        if (strcmp(strData, "ON") == 0)
        {
            state = true;
        }
        else if (strcmp(strData, "OFF") == 0)
        {
            state = false;
        }
        gpio_app_turn_warning(state);
    }

    if (strcmp(strTopic, MQTT_APP_TOPIC_SUB_CO) == 0)
    {
        // Process data received from MQTT (CO)
        setValue(strData);
    }

    free(strTopic);
    free(strData);
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    // int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        g_mqtt_connect_status = MQTT_APP_CONNECT_SUCCESS;
        mqtt_app_send_message(MQTT_APP_MSG_CONNECTED);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        g_mqtt_connect_status = MQTT_APP_CONNECT_NONE;
        mqtt_app_send_message(MQTT_APP_MSG_DISCONNECTED);

        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // mqtt_app_send_message(MQTT_APP_MSG_SUBSCRIBED);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);

        //  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        // printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        // printf("DATA=%.*s\r\n", event->data_len, event->data);
        // if (strncmp(event->data, "send binary please", event->data_len) == 0)
        // {
        //     ESP_LOGI(TAG, "Sending the binary");
        //     // send_binary(client);
        // }

        // mqtt_app_proccess_received_data(event->topic, event->topic_len, event->data, event->data_len);
        // memset(event->data, 0, event->data_len);

        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
        {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        }
        else
        {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/**
 * Configures the MQTT Client access to the Broker
 */
static void mqtt_app_config()
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_APP_BROKER_URI,
            .verification.certificate = (const char *)mqtt_eclipseprojects_io_pem_start},
        .credentials = {.username = MQTT_APP_USER, .authentication.password = MQTT_APP_PASSWORD}};
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
}

/**
 * Main task for the MQTT application
 */
static void mqtt_app_task()
{
    mqtt_app_message_t msg;

    // Config uri, certification for client - broker
    mqtt_app_config();

    // Register the handler events
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    // Send the MQTT_APP_MSG_START
    mqtt_app_send_message(MQTT_APP_MSG_START);
    int msg_id;
    for (;;)
    {
        if (xQueueReceive(mqtt_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgId)
            {
            case MQTT_APP_MSG_START:
                // Start connect MQTT client
                ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));

                break;

            case MQTT_APP_MSG_CONNECTED:

                // // when connected, subscribe the declared topics
                // msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_APP_TOPIC_SUB_CO, 0);
                // msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_APP_TOPIC_SUB_BUZZ, 0);

                break;

            case MQTT_APP_MSG_DISCONNECTED:
                // Stop connect MQTT client
                ESP_ERROR_CHECK(esp_mqtt_client_disconnect(mqtt_client));

                break;

            case MQTT_APP_MSG_RECONNECT:
                // Re-connect MQTT client
                ESP_ERROR_CHECK(esp_mqtt_client_reconnect(mqtt_client));

                break;

            case MQTT_APP_MSG_SUBSCRIBED:
                // msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_APP_TOPIC_PUB, "data", 0, 0, 0);
                // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                break;

            case MQTT_APP_MSG_PUBLISHED:
                char *topic = createString(msg.msgTopic, msg.msgLenTopic);
                char *data = createString(msg.msgData, msg.msgLenData);

                printf(topic);
                printf(data);

                // Calculate the total length of the JSON string
                size_t jsonLength = 24 + msg.msgLenData + msg.msgLenTopic;

                char *buff = (char *)malloc((jsonLength + 2) * sizeof(char)); // Allocate memory for the JSON string + null terminator

                if (buff != NULL)
                {
                    // Use snprintf to ensure null termination and avoid buffer overflow
                    snprintf(buff, jsonLength + 2, "{\"deviceId\":\"%s\",\"data\":\"%s\"}", topic, data);
                     ESP_LOGI(TAG, "%s \n",buff);
                    msg_id = esp_mqtt_client_publish(mqtt_client, topic, buff, jsonLength+1, 0, 0);

                    // Don't forget to free the allocated memory when you are done using the strings
                    free(topic);
                    free(data);
                    free(buff);
                }
                else
                {
                    // Handle memory allocation failure
                    // You may want to log an error or take appropriate action
                }

                break;

            case MQTT_APP_MSG_ONWARNING:
                msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_APP_TOPIC_SUB_BUZZ, "ON", 0, 0, 0);
                break;

            case MQTT_APP_MSG_OFFWARNING:
                msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_APP_TOPIC_SUB_BUZZ, "OFF", 0, 0, 0);
                break;
            default:
                break;
            }
        }
    }
}

/**
 * Start mqtt_app
 */
void mqtt_app_start(void)
{

    if (tHandler_mqtt_app == NULL)
    {
        ESP_LOGI(TAG, "Starting MQTT Application");

        // Create the queue msg
        mqtt_app_queue_handle = xQueueCreate(3, sizeof(mqtt_app_message_t));

        // Start the MQTT application task
        xTaskCreatePinnedToCore(&mqtt_app_task, "mqtt_app_task", MQTT_APP_TASK_STACK_SIZE, NULL, MQTT_APP_TASK_PRIORITY, &tHandler_mqtt_app, MQTT_APP_TASK_CORE_ID);
    }
    else
    {

        ESP_LOGI(TAG, "Reconnect MQTT Application");
        // Send the MQTT_APP_MSG_START
        mqtt_app_send_message(MQTT_APP_MSG_RECONNECT);
    }
}

/**
 * Stop mqtt client when wifi disconnected station mode
 */
void mqtt_app_stop()
{
    if (tHandler_mqtt_app)
    {
        vTaskDelete(tHandler_mqtt_app);
        ESP_LOGI(TAG, "Stopping MQTT Application");
        tHandler_mqtt_app = NULL;
    }
}