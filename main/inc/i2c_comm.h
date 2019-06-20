/* ===== [i2c_comm.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __I2C_COMM_H__
#define __I2C_COMM_H__

/* ===== Dependencies ===== */
#include "driver/i2c.h"

/* ===== Macros of public constants ===== */
#define I2C_ESP_SLAVE_ADDR 0x28
#define I2C_SLAVE_SCL_IO 26 	// gpio for I2C slave clock (SCL)
#define I2C_SLAVE_SDA_IO 25 	// gpio for I2c slave data (SDA)
#define I2C_MASTER_SCL_IO 19	// gpio for I2C slave clock (SCL)
#define I2C_MASTER_SDA_IO 18	// gpio for I2c slave data (SDA)

/* ===== Public structs and enums ===== */

/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: initialize_i2c_master
| ------------------------------------------------------------------
|  Description: it configures the I2C master port using default
|				GPIOs.
|
|  Parameters:
|		-
|
|  Returns:  esp_err_t
*-------------------------------------------------------------------*/
esp_err_t initialize_i2c_master();


/*------------------------------------------------------------------
|  Function: initialize_i2c_slave
| ------------------------------------------------------------------
|  Description: it configures the I2C slave port using default
|				GPIOs.
|
|  Parameters:
|		-
|
|  Returns:  esp_err_t
*-------------------------------------------------------------------*/
esp_err_t initialize_i2c_slave(uint16_t slave_addr);


/*------------------------------------------------------------------
|  Function: i2c_master_task
| ------------------------------------------------------------------
|  Description: FreeRTOS task. It periodically reads the I2C slave,
|				if there is a new command, it passes it to the
|				WiFi task.
|
|  Parameters:
|		- pvParameter: void pointer used as task parameter during
|					   task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void i2c_master_task(void *pvParameter);


/*------------------------------------------------------------------
|  Function: i2c_slave_task
| ------------------------------------------------------------------
|  Description: FreeRTOS task. It periodically checks if the UART
|				task have sent new data. If yes, it write the data
|				in the I2C buffer that the master would read.
|
|  Parameters:
|		- pvParameter: void pointer used as task parameter during
|					   task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void i2c_slave_task(void *pvParameter);

/* ===== Avoid multiple inclusion ===== */
#endif // __I2C_COMM_H__
