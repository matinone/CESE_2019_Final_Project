#include "echo_uart.h"

#include "freertos/FreeRTOS.h"

// UART driver
#include "driver/uart.h"

extern QueueHandle_t queue_echo_to_wifi;

void initialize_uart()
{
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
    	printf("Could not create echo_uart QUEUE.\n");
    }
}

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