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

// queues
QueueHandle_t queue_uart_to_i2c;		// queue to pass data between the ECHO TASK and the I2C SLAVE TASK 
QueueHandle_t queue_i2c_to_wifi;		// queue to pass data between the I2C MASTER TASK and the WIFI TASK 

// event groups
EventGroupHandle_t wifi_event_group;	// event group to synchronize the WIFI TASK with the WIFI DRIVER events

// main application
void app_main()
{
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS (Non Volatile Storage)
	ESP_ERROR_CHECK(nvs_flash_init());
	
	initialize_wifi();
	initialize_uart();

	initialize_i2c_master();
	initialize_i2c_slave(I2C_ESP_SLAVE_ADDR);

	// NOTE: 
	// printf is redirected to the UART and it is thread safe, 
	// therefore there is no need to use mutexes or any other synchronization method during printf calls

	// create the wifi task with the highest priority (the http request should not be interrupted)
	xTaskCreate(&wifi_task, "wifi_task", 2048 + 1024, NULL, 5, NULL);

	// create the rest of the tasks with priority lower than wifi task
	xTaskCreate(&echo_task, "echo_task", 2048, NULL, 4, NULL);
	xTaskCreate(&i2c_master_task, "i2c_master_task", 2048, NULL, 4, NULL);
	xTaskCreate(&i2c_slave_task, "i2c_slave_task", 2048, NULL, 4, NULL);

	// vTaskStartScheduler is called in the startup code before app_main is executed (see start_cpu0 function in ESP-IDF components/esp32/cpu_start.c)
}
