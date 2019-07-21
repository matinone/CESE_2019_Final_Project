/* ===== [echo_uart.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "echo_uart.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "command_processor.h"

/* ===== Macros of private constants ===== */
#define UART_RX_CHECK_TIME_MS 100
#define UART_TX_PIN 1
#define UART_RX_PIN 3

/* ===== Declaration of private or external variables ===== */
extern QueueHandle_t queue_command_processor_rx;
QueueHandle_t queue_uart_tx;

static const char* TAG = "UART_TASK";

/* ===== Prototypes of private functions ===== */


/* ===== Implementations of public functions ===== */
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
    uart_set_pin(UART_NUM_0, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // create a queue capable of containing 5 uint8_t values
    queue_uart_tx = xQueueCreate(5, sizeof(uint8_t));
    if (queue_uart_tx == NULL)
    {
        printf("Could not create queue_uart_tx.\n");
    }
}

void echo_task(void *pvParameter)
{   
    uint8_t* uart_rcv_buffer = (uint8_t*) malloc(1);
    int rcv_len;
    command_type_t command_type;
    rx_command_t uart_command;
    uart_command.rx_id = UART_RX;

    BaseType_t xStatus;

    while (1)
    {
        // read data from the UART
        rcv_len = uart_read_bytes(UART_NUM_0, (uint8_t*)uart_rcv_buffer, 1, 20 / portTICK_RATE_MS);
        if (rcv_len > 0)
        {
            ESP_LOGI(TAG, "Received from UART: %d\n", *uart_rcv_buffer - '0');

            // send the received value to the queue (wait 1000ms if the queue is full)
            uart_command.command = *uart_rcv_buffer - '0';
            xStatus = xQueueSendToBack(queue_command_processor_rx, &uart_command, 1000 / portTICK_RATE_MS);
            if (xStatus != pdPASS)
            {
                printf("Could not send the data to the queue.\n");
            }
        }

        // read data from the tx queue
        xStatus = xQueueReceive(queue_uart_tx, &command_type, 100 / portTICK_RATE_MS);
        if (xStatus == pdPASS) 
        {
            ESP_LOGI(TAG, "Received from Command Processor: %d\n", command_type);
        }

        vTaskDelay(UART_RX_CHECK_TIME_MS / portTICK_RATE_MS);
    }
}

/* ===== Implementations of private functions ===== */
