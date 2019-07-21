/* ===== [main.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

// dependencies
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"
#include "echo_uart.h"
#include "i2c_master.h"
#include "i2c_slave.h"

#include "http_tasks.h"
#include "tls_https_tasks.h"
#include "mqtt.h"

#include "ble_server.h"
#include "command_processor.h"

// main application
void app_main()
{
	esp_err_t return_value;

	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

    // initialize NVS (Non Volatile Storage), restart system if the initialization fails
    return_value = nvs_flash_init();
    if (return_value == ESP_ERR_NVS_NO_FREE_PAGES || return_value == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        return_value = nvs_flash_init();
    }
    ESP_ERROR_CHECK(return_value);

	initialize_wifi();
	initialize_uart();
	initialize_command_processor();

	// do not use I2C for now
	// initialize_i2c_master();
	// initialize_i2c_slave(I2C_ESP_SLAVE_ADDR);

	// NOTE: 
	// printf is redirected to the UART and it is thread safe, 
	// therefore there is no need to use mutexes or any other synchronization method during printf calls

	// create wifi tasks with the highest priority (http requests should not be interrupted)
	xTaskCreate(&wifi_tx_task, "wifi_tx_task", 2048, NULL, 6, NULL);
	xTaskCreate(&wifi_rx_cmd_task, "wifi_rx_cmd_task", 2048, NULL, 6, NULL);

	// xTaskCreate(&wifi_secure_tx_task, "wifi_secure_tx_task", 2048*3, NULL, 6, NULL);
	// xTaskCreate(&wifi_secure_rx_cmd_task, "wifi_secure_rx_cmd_task", 2048*3, NULL, 6, NULL);

	// xTaskCreate(&mqtt_publish_task, "mqtt_publish_task", 2048, NULL, 6, NULL);
	// xTaskCreate(&mqtt_rx_task, "mqtt_rx_task", 2048, NULL, 6, NULL);

	// create the rest of the tasks with priority lower than wifi task
	xTaskCreate(&command_processor_task, "command_processor_task", 2048, NULL, 5, NULL);
	xTaskCreate(&echo_task, "echo_task", 2048, NULL, 4, NULL);
	
	// do not use I2C for now
	// xTaskCreate(&i2c_master_task, "i2c_master_task", 2048, NULL, 4, NULL);
	// xTaskCreate(&i2c_slave_task, "i2c_slave_task", 2048, NULL, 4, NULL);

	// vTaskStartScheduler is called in the startup code before app_main is executed (see start_cpu0 function in ESP-IDF components/esp32/cpu_start.c)
}
