#include "driver/i2c.h"

#define I2C_ESP_SLAVE_ADDR 0x28

esp_err_t i2c_master_init();
esp_err_t i2c_slave_init(uint16_t slave_addr);
