#include "time_sntp.h"

#include <string>
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
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
static const char *TAG = "time_sntp";
bool setTimeAlwaysOnReboot = true;
static std::string timeZone = "";
static std::string timeServer = "undefined";
// Set new time-zone
void setTimeZone(std::string _tzstring)
{
    setenv("TZ", _tzstring.c_str(), 1);
    tzset();
    printf("TimeZone set to %s\n", _tzstring.c_str());
    _tzstring = "Time zone set to " + _tzstring;
}

// Get time as String
std::string getTimeString(const char *frm)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    char strftime_buf[64];
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), frm, &timeinfo);

    std::string result(strftime_buf);
    return result;
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    // sntp_setoperatingmode(SNTP_OPMODE_POLL);
    // sntp_setservername(0, "pool.ntp.org");
    // //    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    // sntp_init();
    timeServer = "pool.ntp.org";
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(timeServer.c_str());
    esp_netif_sntp_init(&config);
    timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
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

// Settup timezone
// Notice: Must connect global internet first
void setupTime()
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if ((timeinfo.tm_year < (2016 - 1900)) || setTimeAlwaysOnReboot)
    {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        initialize_sntp();
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    char strftime_buf[64];

    setTimeZone("CST+7"); // VietName time zone
    // setTimeZone("CET-1CEST,M3.5.0,M10.5.0/3");

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in VietNam is: %s", strftime_buf);

    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d_%H:%M", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in VietNam is: %s", strftime_buf);

    std::string zw = getTimeString("%Y%m%d-%H%M%S");
    printf("timeist %s\n", zw.c_str());
}
