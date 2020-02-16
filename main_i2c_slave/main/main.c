#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "slave_sim_task.h"

// main application
void app_main()
{
	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	xTaskCreate(&slave_sim_task, "slave_sim_task", 1024 * 2, NULL, 4, NULL);

	// vTaskStartScheduler is called in the startup code before app_main is executed
	// (see start_cpu0 function in ESP-IDF components/esp32/cpu_start.c)
}
