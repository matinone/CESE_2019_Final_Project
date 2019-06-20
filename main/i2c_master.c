/* ===== [i2c_master.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "i2c_master.h"
#include "i2c_slave.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

/* ===== Macros of private constants ===== */
#define DATA_LENGTH 3                       // data buffer length          

#define I2C_MASTER_NUM I2C_NUM_1
#define I2C_MASTER_FREQ_HZ 100000           //I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0         // master does not need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0         // master does not need buffer

#define ACK_CHECK_EN 0x1                    // I2C master will check ack from slave
#define ACK_CHECK_DIS 0x0                   // I2C master will not check ack from slave
#define ACK_VAL 0x0                         // I2C ack value
#define NACK_VAL 0x1                        // I2C nack value

#define COMMAND_START   's'
#define COMMAND_END     'e'
#define COMMAND_LENGTH  3

/* ===== Declaration of private or external variables ===== */
extern QueueHandle_t queue_i2c_to_wifi;
static const char* TAG = "I2C_MASTER_TASK";

/* ===== Prototypes of private functions ===== */
static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint16_t slave_address, uint8_t *data_rd, size_t size);

/* ===== Implementations of public functions ===== */
esp_err_t initialize_i2c_master()
{
    // create a queue capable of containing 5 char values
    queue_i2c_to_wifi = xQueueCreate(5, sizeof(uint8_t));
    if (queue_i2c_to_wifi == NULL)
    {
        printf("Could not create uart_to_i2c QUEUE.\n");
    }

    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t i2c_master_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_MASTER_FREQ_HZ,
        },
    };

    i2c_param_config(i2c_master_port, &i2c_master_config);
    return i2c_driver_install(i2c_master_port, i2c_master_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


void i2c_master_task(void *pvParameter)
{
    int ret;
    uint8_t* data_rd = (uint8_t*)malloc(DATA_LENGTH);
    BaseType_t xStatus;

    while (1)   
    {
        // data_rd = NULL;
        ret = i2c_master_read_slave(I2C_MASTER_NUM, I2C_ESP_SLAVE_ADDR, data_rd, DATA_LENGTH);
        
        if (ret == ESP_ERR_TIMEOUT) 
        {
            printf("I2C Master Read Slave TIMEOUT\n");
        } 
        else if (ret == ESP_OK) 
        {   
            if (*(data_rd) == COMMAND_START && *(data_rd + 2) == COMMAND_END)
            {
                ESP_LOGI(TAG, "I2C Master Task read from slave the value: %c\n", *(data_rd + 1));
                xStatus = xQueueSendToBack(queue_i2c_to_wifi, data_rd + 1, 0);
                if (xStatus != pdPASS)
                {
                    printf("Could not send the data to the queue.\n");
                }
            }
        } 
        else 
        {
            printf("Master Read Slave error: %s\n", esp_err_to_name(ret));
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}


/* ===== Implementations of private functions ===== */
static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint16_t slave_address, uint8_t *data_rd, size_t size)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
