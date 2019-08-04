#include "nvs_storage.h"

#include "esp_partition.h"
#include "nvs_flash.h"
#include "nvs.h"

// NVS handler
nvs_handle storage_nvs_handler;

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
        printf("Could not initialize NVS.\n");
        return return_value;
    }

    if (storage == INCLUDE_NVS_STORAGE)
    {
        // open the partition in RW mode
        return_value = nvs_open("storage", NVS_READWRITE, &storage_nvs_handler);
        if (return_value != ESP_OK) 
        {
            
            printf("Unable to open NVS\n");
            return return_value;
        }
        printf("NVS successfully opened\n");
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