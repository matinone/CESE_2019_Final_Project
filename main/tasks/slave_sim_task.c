/* ===== [slave_sim_task.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "inc/slave_sim_task.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

/* ===== Macros of private constants ===== */
#define I2C_SLAVE_NUM           I2C_NUM_0
#define I2C_SLAVE_TX_BUF_LEN    1024
#define I2C_SLAVE_RX_BUF_LEN    1024

#define COMMAND_START           's'
#define COMMAND_END             'e'
#define COMMAND_LENGTH          3

/* ===== Declaration of private or external variables ===== */
static const char *TAG = "I2C_SLAVE_SIM_TASK";

/* ===== Prototypes of private functions ===== */
esp_err_t initialize_i2c_slave(uint16_t slave_addr);
uint8_t check_frame_format(uint8_t* frame);

/* ===== Implementations of public functions ===== */
void slave_sim_task(void *pvParameter)
{
    uint8_t command_frame[COMMAND_LENGTH];
    size_t sent_size;
    int32_t slave_read_buffer_size;
    esp_err_t error_state;

    // initialize I2C slave
    error_state = initialize_i2c_slave(I2C_ESP_SLAVE_ADDR);
    if (error_state != ESP_OK)
    {
        ESP_LOGE(TAG, "Error initializing I2C slave.");
        // do something about this
    }

    // initially send something to start the whole process
    command_frame[0] = COMMAND_START;
    command_frame[1] = 'A';
    command_frame[2] = COMMAND_END;
    sent_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, command_frame, COMMAND_LENGTH, 500 / portTICK_RATE_MS);

    while (1)
    {
        // read data from slave buffer
        slave_read_buffer_size = i2c_slave_read_buffer(I2C_SLAVE_NUM, command_frame, COMMAND_LENGTH, 1000 / portTICK_RATE_MS);
        if (slave_read_buffer_size > 0 && check_frame_format(command_frame))
        {
            ESP_LOGI(TAG, "I2C Slave Sim Task read from slave buffer: %d\n", command_frame[1]);
            command_frame[1]++;
            sent_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, command_frame, COMMAND_LENGTH, 500 / portTICK_RATE_MS);
            if (sent_size == 0)    {
                printf("I2C slave buffer is full, UNABLE to write\n");
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}


/* ===== Implementations of private functions ===== */
esp_err_t initialize_i2c_slave(uint16_t slave_addr)
{
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

uint8_t check_frame_format(uint8_t* frame)
{
    return (frame[0] == COMMAND_START && frame[2] == COMMAND_END);
}
