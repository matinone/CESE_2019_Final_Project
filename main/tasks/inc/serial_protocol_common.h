/* ===== [serial_protocol_common.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __SERIAL_PROTOCOL_COMMON_H__
#define __SERIAL_PROTOCOL_COMMON_H__

/* ===== Dependencies ===== */
#include <stdint.h>

/* ===== Macros of public constants ===== */
#define I2C_MASTER_SCL_IO   19	        // gpio for I2C master clock (SCL)
#define I2C_MASTER_SDA_IO   18	        // gpio for I2c master data (SDA)
#define I2C_SLAVE_SCL_IO    26 	        // gpio for I2C slave clock (SCL)
#define I2C_SLAVE_SDA_IO    25 	        // gpio for I2c slave data (SDA)
#define I2C_ESP_SLAVE_ADDR  0x28

#define COMMAND_START       's'
#define COMMAND_END         'e'
#define COMMAND_LENGTH      3

/* ===== Public structs and enums ===== */

/* ===== Prototypes of public functions ===== */
uint8_t check_frame_format(uint8_t* frame);

/* ===== Avoid multiple inclusion ===== */
#endif // __SERIAL_PROTOCOL_COMMON_H__
