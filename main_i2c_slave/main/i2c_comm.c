#include "i2c_comm.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define DATA_LENGTH 3                       // data buffer length

#define I2C_SLAVE_SCL_IO 26                 // gpio for I2C slave clock (SCL)
#define I2C_SLAVE_SDA_IO 25                 // gpio for I2c slave data (SDA)
#define I2C_SLAVE_NUM I2C_NUM_0             
#define I2C_SLAVE_TX_BUF_LEN 1024           
#define I2C_SLAVE_RX_BUF_LEN 1024           

#define ACK_CHECK_EN 0x1                    // I2C master will check ack from slave
#define ACK_CHECK_DIS 0x0                   // I2C master will not check ack from slave
#define ACK_VAL 0x0                         //I2C ack value
#define NACK_VAL 0x1                        //I2C nack value

#define COMMAND_START   's'
#define COMMAND_END     'e'
#define COMMAND_LENGTH  3

extern QueueHandle_t queue_uart_to_i2c;


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


// i2c slave task
void i2c_slave_task(void *pvParameter)
{
    uint8_t queue_rcv_value;
    uint8_t command_frame[COMMAND_LENGTH] = {COMMAND_START, 0, COMMAND_END};
    size_t d_size;
    BaseType_t xStatus;

    while (1)
    {
        // read data from the queue
        xStatus = xQueueReceive( queue_uart_to_i2c, &queue_rcv_value,  20 / portTICK_RATE_MS);
        if (xStatus == pdPASS)
        {   
            command_frame[1] = queue_rcv_value;
            printf("\nI2C TASK received from ECHO TASK: %c\n", command_frame[1]);

            d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, command_frame, COMMAND_LENGTH, 500 / portTICK_RATE_MS);
            if (d_size == 0)
            {
                printf("I2C slave buffer is full, UNABLE to write\n");
            }
            else
            {
                printf("%c successfully written to I2C slave buffer\n", command_frame[1]);
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);

    }   // while(1)
}