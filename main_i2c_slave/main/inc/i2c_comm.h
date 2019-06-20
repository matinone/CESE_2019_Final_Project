#include "driver/i2c.h"

#define I2C_ESP_SLAVE_ADDR 0x28

esp_err_t initialize_i2c_slave(uint16_t slave_addr);

void i2c_slave_task(void *pvParameter);