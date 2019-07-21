/* ===== [i2c_slave.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "i2c_slave.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

/* ===== Macros of private constants ===== */
#define I2C_SLAVE_NUM I2C_NUM_0             
#define I2C_SLAVE_TX_BUF_LEN 1024           
#define I2C_SLAVE_RX_BUF_LEN 1024           

#define COMMAND_START   's'
#define COMMAND_END     'e'
#define COMMAND_LENGTH  3

/* ===== Declaration of private or external variables ===== */
QueueHandle_t queue_i2c_slave_tx;
static const char *TAG = "I2C_SLAVE_TASK";

/* ===== Prototypes of private functions ===== */

/* ===== Implementations of public functions ===== */
esp_err_t initialize_i2c_slave(uint16_t slave_addr)
{
    // create a queue capable of containing 5 char values
    queue_i2c_slave_tx = xQueueCreate(5, sizeof(uint8_t));
    if (queue_i2c_slave_tx == NULL)
    {
        printf("Could not create queue_i2c_slave_tx.\n");
    }

    int i2c_slave_port = I2C_SLAVE_NUM;
    i2c_config_t i2c_slave_config = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .slave = {
            .addr_10bit_en = 0,
            .slave_addr = slave_addr,
        },
    };

    i2c_param_config(i2c_slave_port, &i2c_slave_config);
    return i2c_driver_install(i2c_slave_port, i2c_slave_config.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
}


void i2c_slave_task(void *pvParameter)
{
    uint8_t queue_rcv_value;
    uint8_t command_frame[COMMAND_LENGTH] = {COMMAND_START, 0, COMMAND_END};
    size_t d_size;
    BaseType_t xStatus;

    while (1)
    {
        // read data from the queue
        xStatus = xQueueReceive( queue_i2c_slave_tx, &queue_rcv_value,  20 / portTICK_RATE_MS);
        if (xStatus == pdPASS)
        {   
            command_frame[1] = queue_rcv_value;
            ESP_LOGI(TAG, "I2C TASK received from UART TASK: %c\n", command_frame[1]);

            d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, command_frame, COMMAND_LENGTH, 500 / portTICK_RATE_MS);
            if (d_size == 0)
            {
                printf("I2C slave buffer is full, UNABLE to write\n");
            }
            else
            {
                ESP_LOGI(TAG, "%c successfully written to I2C slave buffer\n", command_frame[1]);
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);

    }   // while(1)
}


/* ===== Implementations of private functions ===== */
