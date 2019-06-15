#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "nvs_flash.h"

// wifi
#include "wifi.h"
#include "echo_uart.h"
#include "i2c_comm.h"

// Queues
QueueHandle_t queue_echo_to_wifi;

// Event groups
EventGroupHandle_t wifi_event_group;

// Main application
void app_main()
{
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	initialize_wifi();
	initialize_uart();

	initialize_i2c_master();
	initialize_i2c_slave(I2C_ESP_SLAVE_ADDR);

	// start the wifi task
	xTaskCreate(&wifi_task, "wifi_task", 2048, NULL, 5, NULL);

	// start echo task with priority lower than wifi task
	xTaskCreate(&echo_task, "echo_task", 2048, NULL, 4, NULL);
}
