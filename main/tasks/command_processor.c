/* ===== [command_processor.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "command_processor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"


/* ===== Macros of private constants ===== */
#define CMD_RX_CHECK_TIME_MS 500

/* ===== Declaration of private or external variables ===== */
QueueHandle_t queue_command_processor_rx;
extern QueueHandle_t queue_uart_tx;
// static const char* TAG = "CMD_PROCESSOR_TASK";

/* ===== Prototypes of private functions ===== */
char* translate_rx_module(rx_module_t module);
char* translate_command_type(command_type_t command);
QueueHandle_t* get_module_queue(rx_module_t module);


/* ===== Implementations of public functions ===== */
int8_t initialize_command_processor()
{
    // create a queue capable of containing 5 rx_command_t values
    queue_command_processor_rx = xQueueCreate(5, sizeof(rx_command_t));
    if (queue_command_processor_rx == NULL)
    {
        printf("Could not Command Processor RX QUEUE.\n");
        return -1;
    }
    return 0;
}

void command_processor_task(void *pvParameter)
{   
    rx_command_t current_command;
    BaseType_t xStatus;

    while (1)
    {
        // read data from the queue
        xStatus = xQueueReceive(queue_command_processor_rx, &current_command,  portMAX_DELAY);
        if (xStatus == pdPASS) 
        {
            printf("Received command %s from %s.\n", translate_command_type(current_command.command), 
                                                     translate_rx_module(current_command.rx_id));

            // do something here depending on the received command
            switch(current_command.command)
            {
                case CMD_ECHO:
                    xStatus = xQueueSendToBack(*(get_module_queue(current_command.rx_id)), &current_command.command, 1000 / portTICK_RATE_MS);
                    if (xStatus != pdPASS)
                    {
                        printf("Could not send the data to the queue.\n");
                    }
                    break;
                default:
                    break;
            }
        }
        
        vTaskDelay(CMD_RX_CHECK_TIME_MS / portTICK_RATE_MS);
    }
}

/* ===== Implementations of private functions ===== */
char* translate_rx_module(rx_module_t module)
{
    switch(module)
    {
        case WIFI_RX:
            return "WIFI_RX";
        case BLE_SERVER:
            return "BLE_SERVER";
        case UART_RX:
            return "UART_RX";
        case I2C_SLAVE:
            return "I2C_SLAVE";
        default:
            return "UNKNOWN";
    }
}

char* translate_command_type(command_type_t command)
{
    switch(command)
    {
        case CMD_START:
            return "CMD_START";
        case CMD_STOP:
            return "CMD_STOP";
        case CMD_STATUS:
            return "CMD_STATUS";
        case CMD_RESTART:
            return "CMD_RESTART";
        case CMD_WIFI:
            return "CMD_WIFI";
        case CMD_BLE:
            return "CMD_BLE";
        case CMD_ECHO:
            return "CMD_ECHO";
        default:
            return "UNKNOWN";
    }
}

QueueHandle_t* get_module_queue(rx_module_t module)
{
    switch(module)
    {
        case UART_RX:
            return &queue_uart_tx;
        default:
            return NULL;
    }
}
