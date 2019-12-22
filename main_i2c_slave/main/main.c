#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "echo_uart.h"
#include "i2c_slave.h"


// Main application
void app_main()
{

	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	initialize_uart();
	initialize_i2c_slave(I2C_ESP_SLAVE_ADDR);

	// NOTE: 
	// printf is redirected to the UART and it is thread safe, 
	// therefore there is no need to use mutexes or any other synchronization method during printf calls

	
}
