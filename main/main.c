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

#ifdef CONFIG_HTTP
	#include "http_tasks.h"
	#define WIFI_MODULE HTTP_RX
#else
	#ifdef CONFIG_HTTPS
		#include "tls_https_tasks.h"
		#define WIFI_MODULE HTTPS_RX
	#else
		#include "mqtt.h"
		#define WIFI_MODULE MQTT_RX
	#endif
#endif

#include "ble_server.h"
#include "command_processor.h"
#include "nvs_storage.h"

// main application
void app_main()
{
	esp_err_t return_value;
	rx_module_t wifi_module = WIFI_MODULE;

	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS (Non Volatile Storage), restart system if the initialization fails
	return_value = init_nvs_storage(INCLUDE_NVS_STORAGE);
	ESP_ERROR_CHECK(return_value);

	wifi_credential_t wifi_credential = {
		.ssid = CONFIG_WIFI_SSID,
		.password = CONFIG_WIFI_PASSWORD,
	};
	set_current_wifi_credentials(&wifi_credential);
	initialize_wifi(WIFI_FIRST_CONFIG, WIFI_MODE_APSTA, &wifi_credential);
	initialize_uart();
	initialize_command_processor(wifi_module);

	// do not use I2C for now
	// initialize_i2c_master();
	// initialize_i2c_slave(I2C_ESP_SLAVE_ADDR);

	// NOTE: 
	// printf is redirected to the UART and it is thread safe, 
	// therefore there is no need to use mutexes or any other synchronization method during printf calls

	// create HTTP/HTTPS/MQTT tasks (depending on the compiler options) with the highest priority
	#ifdef CONFIG_HTTP
		xTaskCreate(&wifi_tx_task, "wifi_tx_task", 2048, NULL, 6, NULL);
		xTaskCreate(&wifi_rx_cmd_task, "wifi_rx_cmd_task", 2048, NULL, 6, NULL);
	#else
		#ifdef CONFIG_HTTPS
		xTaskCreate(&wifi_secure_tx_task, "wifi_secure_tx_task", 2048*3, NULL, 6, NULL);
		xTaskCreate(&wifi_secure_rx_cmd_task, "wifi_secure_rx_cmd_task", 2048*3, NULL, 6, NULL);
		#else
		xTaskCreate(&mqtt_publish_task, "mqtt_publish_task", 2048, NULL, 6, NULL);
		xTaskCreate(&mqtt_rx_task, "mqtt_rx_task", 2048, NULL, 6, NULL);
		#endif
	#endif

	// create the rest of the tasks with priority lower than wifi task
	xTaskCreate(&command_processor_task, "command_processor_task", 2048, NULL, 5, NULL);
	xTaskCreate(&echo_task, "echo_task", 2048, NULL, 4, NULL);
	
	// do not use I2C for now
	// xTaskCreate(&i2c_master_task, "i2c_master_task", 2048, NULL, 4, NULL);
	// xTaskCreate(&i2c_slave_task, "i2c_slave_task", 2048, NULL, 4, NULL);

	// vTaskStartScheduler is called in the startup code before app_main is executed (see start_cpu0 function in ESP-IDF components/esp32/cpu_start.c)
}
