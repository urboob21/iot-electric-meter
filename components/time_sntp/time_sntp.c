#include "time_sntp.h"

#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

static const char *TAG = "time_sntp";
bool setTimeAlwaysOnReboot = true;
static char timeZone[64] = "";
static char timeServer[64] = "undefined";

void setTimeZone(char *tzstring)
{
    setenv("TZ", tzstring, 1);
    tzset();
    printf("TimeZone set to %s\n", tzstring);
    sprintf(timeZone, "Time zone set to %s", tzstring);
}

char* getTimeString(const char *frm)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    char strftime_buf[64];
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), frm, &timeinfo);

    char *result = (char *)malloc(strlen(strftime_buf) + 1);
    if (result) {
        strcpy(result, strftime_buf);
    }
    return result;
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(timeServer);
    esp_netif_sntp_init(&config);
    setTimeZone(timeZone);
}

static void obtain_time(void)
{
    time_t now = 0;
    struct tm timeinfo = {};
    int retry = 0;
    const int retry_count = 10;
    time(&now);
    localtime_r(&now, &timeinfo);
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    time(&now);
    localtime_r(&now, &timeinfo);
}

void setupTime()
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if ((timeinfo.tm_year < (2016 - 1900)) || setTimeAlwaysOnReboot)
    {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        initialize_sntp();
        obtain_time();
        time(&now);
    }
    char strftime_buf[64];

    setTimeZone("CST+7"); // VietName time zone

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in VietNam is: %s", strftime_buf);

    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d_%H:%M", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in VietNam is: %s", strftime_buf);

    char *zw = getTimeString("%Y%m%d-%H%M%S");
    if (zw) {
        printf("timeist %s\n", zw);
        free(zw);
    }
}
