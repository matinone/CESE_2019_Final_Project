#include "i2c_comm.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define DATA_LENGTH 512                     // data buffer length
#define RW_TEST_LENGTH 128                  // data length for r/w test, [0,DATA_LENGTH]

#define I2C_SLAVE_SCL_IO 26                 // gpio for I2C slave clock (SCL)
#define I2C_SLAVE_SDA_IO 25                 // gpio for I2c slave data (SDA)
#define I2C_SLAVE_NUM I2C_NUM_0             
#define I2C_SLAVE_TX_BUF_LEN (2 * DATA_LENGTH)             
#define I2C_SLAVE_RX_BUF_LEN (2 * DATA_LENGTH)             

#define I2C_MASTER_SCL_IO 19                // gpio for I2C slave clock (SCL)
#define I2C_MASTER_SDA_IO 18                // gpio for I2c slave data (SDA)
#define I2C_MASTER_NUM I2C_NUM_1
#define I2C_MASTER_FREQ_HZ 100000           //I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0         // master does not need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0         // master does not need buffer


esp_err_t initialize_i2c_master()
{
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

