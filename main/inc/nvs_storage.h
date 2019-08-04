#include <stdint.h>
#include "esp_err.h"

#define INCLUDE_NVS_STORAGE 1
#define WIFI_SSID_NVS_KEY "wifi_ssid"
#define WIFI_PASSWORD_NVS_KEY "wifi_password"

esp_err_t init_nvs_storage(uint8_t storage);
char* get_nvs_string_value(char* string_key);
esp_err_t set_nvs_string_value(char* string_key, char* string_value);
