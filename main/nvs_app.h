#ifndef MAIN_NVS_APP_H
#define MAIN_NVS_APP_H
/**
 * Saves station mode Wifi credentials to NVS
 * @return ESP_OK if successful.
 */
esp_err_t nvs_app_save_sta_creds();

/**
 * Loads the previously saved credentials from NVS.
 * @return true if previously saved credentials were found.
 */
bool nvs_app_load_sta_creds();

 /* Clears station mode credentials from NVS
 * @return ESP_OK if successful.
 */
esp_err_t nvs_app_clear_sta_creds();

#endif