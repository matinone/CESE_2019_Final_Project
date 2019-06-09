#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

// UART driver
#include "driver/uart.h"

// wifi
#include "wifi.h"

// Queues
QueueHandle_t queue_echo_to_wifi;

// Event group
EventGroupHandle_t wifi_event_group;

void echo_task(void *pvParameter)
{	
	uint8_t* uart_rcv_buffer = (uint8_t*) malloc(1);
	int rcv_len;
	BaseType_t xStatus;

	while (1)
	{
		// read data from the UART
		rcv_len = uart_read_bytes(UART_NUM_0, (uint8_t*)uart_rcv_buffer, 1, 20 / portTICK_RATE_MS);
		if (rcv_len > 0)
		{
			printf("\nReceived from UART: %d (Echo Task)\n", *uart_rcv_buffer);
			// send the received value to the queue (wait 0ms if the queue is empty)
			xStatus = xQueueSendToBack( queue_echo_to_wifi, uart_rcv_buffer, 0 );
			if (xStatus != pdPASS)
			{
				printf("Could not send the data to the queue.\n");
			}
		}
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}


// Main application
void app_main()
{
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	initialize_wifi();

	// configure the UART0 controller
	uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // create a queue capable of containing 5 char values
    queue_echo_to_wifi = xQueueCreate(5, sizeof(uint8_t));
    if (queue_echo_to_wifi == NULL)
    {
    	printf("Could not create QUEUE.\n");
    }

	// start the main task
	xTaskCreate(&wifi_task, "wifi_task", 2048, NULL, 5, NULL);

	// start echo task with priority lower than wifi task
	xTaskCreate(&echo_task, "echo_task", 2048, NULL, 4, NULL);
}
