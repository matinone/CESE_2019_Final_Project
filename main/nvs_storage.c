/* ===== [nvs_storage.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "nvs_storage.h"

#include "esp_partition.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_log.h"

/* ===== Macros of private constants ===== */


/* ===== Declaration of private or external variables ===== */
nvs_handle storage_nvs_handler; // NVS handler
static const char *TAG  = "NVS_STORAGE";

/* ===== Prototypes of private functions ===== */


/* ===== Implementations of public functions ===== */
esp_err_t init_nvs_storage(uint8_t storage)
{
    // initialize NVS
    esp_err_t return_value = nvs_flash_init();
    if (return_value == ESP_ERR_NVS_NO_FREE_PAGES || return_value == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        nvs_flash_erase();
        return_value = nvs_flash_init();
    }

    if (return_value != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not initialize NVS.");
        return return_value;
    }

    if (storage == INCLUDE_NVS_STORAGE)
    {
        // open the partition in RW mode
        return_value = nvs_open("storage", NVS_READWRITE, &storage_nvs_handler);
        if (return_value != ESP_OK) 
        {
            
            ESP_LOGE(TAG, "Unable to open NVS.");
            return return_value;
        }
        ESP_LOGI(TAG, "NVS successfully opened.");
    }
    
    return return_value;
}

char* get_nvs_string_value(char* string_key)
{
    esp_err_t nvs_error;
    size_t string_size;
    nvs_error = nvs_get_str(storage_nvs_handler, string_key, NULL, &string_size);
    char* nvs_string_value = malloc(string_size);
    if (nvs_string_value != NULL)
    {
        nvs_error = nvs_get_str(storage_nvs_handler, string_key, nvs_string_value, &string_size);
    }
    
    if (nvs_error != ESP_OK)
    {
        return NULL;
    }

    return nvs_string_value;
}

esp_err_t set_nvs_string_value(char* string_key, char* string_value)
{
    return nvs_set_str(storage_nvs_handler, string_key, string_value);
}

/* ===== Implementations of private functions ===== */
