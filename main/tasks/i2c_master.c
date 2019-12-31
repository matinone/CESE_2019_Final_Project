/* ===== [i2c_master.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "i2c_master.h"
#include "serial_protocol_common.h"
#include "command_processor.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

/* ===== Macros of private constants ===== */
#define I2C_MASTER_FREQ_HZ          100000      //I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0           // master does not need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0           // master does not need buffer

#define ACK_CHECK_EN                0x1         // I2C master will check ack from slave
#define ACK_CHECK_DIS               0x0         // I2C master will not check ack from slave
#define ACK_VAL                     0x0         // I2C ack value
#define NACK_VAL                    0x1         // I2C nack value

/* ===== Declaration of private or external variables ===== */
static const char* TAG = "I2C_MASTER_TASK";
extern QueueHandle_t queue_command_processor_rx;
QueueHandle_t queue_i2c_master;

/* ===== Prototypes of private functions ===== */


/* ===== Implementations of public functions ===== */
esp_err_t initialize_i2c_master()
{
    // create a queue capable of containing 5 uint8_t values
    queue_i2c_master = xQueueCreate(5, sizeof(uint8_t));
    if (queue_i2c_master == NULL)
    {
        printf("Could not create queue_i2c_master QUEUE.\n");
    }

    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t i2c_master_config = {
        .mode           = I2C_MODE_MASTER,
        .sda_io_num     = I2C_MASTER_SDA_IO,
        .sda_pullup_en  = GPIO_PULLUP_ENABLE,
        .scl_io_num     = I2C_MASTER_SCL_IO,
        .scl_pullup_en  = GPIO_PULLUP_ENABLE,
        .master         = {
            .clk_speed  = I2C_MASTER_FREQ_HZ,
        },
    };

    i2c_param_config(i2c_master_port, &i2c_master_config);
    return i2c_driver_install(i2c_master_port, i2c_master_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


void i2c_master_task(void *pvParameter)
{
    int ret;
    uint8_t data_to_slave[COMMAND_FRAME_LENGTH];
    uint8_t data_from_slave[COMMAND_FRAME_LENGTH];
    uint8_t command_received;
    BaseType_t xStatus;

    rx_command_t ack_command;
    ack_command.rx_id = I2C_MASTER_MOD;


    data_to_slave[0] = COMMAND_FRAME_START;
    data_to_slave[2] = COMMAND_FRAME_END;

    while (1)   
    {
        // read data from the command processor queue
        xStatus = xQueueReceive(queue_i2c_master, &command_received, 500 / portTICK_RATE_MS);
        if (xStatus == pdPASS)
        {
            ack_command.command = CMD_SLAVE_FAIL;
            ESP_LOGI(TAG, "Received %d from Command Processor, sending it to slave\n", command_received);
            data_to_slave[1] = command_received;
            ret = i2c_master_write_slave(I2C_MASTER_NUM, I2C_ESP_SLAVE_ADDR, data_to_slave, COMMAND_FRAME_LENGTH);
            if(ret == ESP_OK)
            {
                // read ack from the slave
                ret = i2c_master_read_slave(I2C_MASTER_NUM, I2C_ESP_SLAVE_ADDR, data_from_slave, COMMAND_FRAME_LENGTH);
                if (ret == ESP_OK && check_frame_format(data_from_slave))
                {
                    if(data_from_slave[1] == CMD_SLAVE_OK)
                    {
                        // ESP_LOGI(TAG, "Received ACK from slave for command %d\n", command_received);
                        ack_command.command = CMD_SLAVE_OK;
                    }
                }
            }

            // send back ack status to command processor
            xStatus = xQueueSendToBack(queue_command_processor_rx, &ack_command, 100 / portTICK_RATE_MS);
            if (xStatus != pdPASS)
            {
                ESP_LOGI(TAG, "Could not send ACK back to the Command Processor.\n");
            }

        }

        // read data from the slave
        memset(data_from_slave, 0, COMMAND_FRAME_LENGTH); // clear the rx buffer
        ret = i2c_master_read_slave(I2C_MASTER_NUM, I2C_ESP_SLAVE_ADDR, data_from_slave, COMMAND_FRAME_LENGTH);
        if (ret == ESP_OK && check_frame_format(data_from_slave))
        {
            ESP_LOGI(TAG, "Received %c from slave\n", data_from_slave[1]);
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint16_t slave_address, uint8_t *data_rd, size_t size)
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

esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint16_t slave_address, uint8_t *data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


/* ===== Implementations of private functions ===== */
