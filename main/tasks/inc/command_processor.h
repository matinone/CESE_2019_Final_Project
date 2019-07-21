/* ===== [command_processor.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Avoid multiple inclusion ===== */
#ifndef __COMMAND_PROCESSOR_H__
#define __COMMAND_PROCESSOR_H__

/* ===== Dependencies ===== */
#include <stdint.h>

/* ===== Macros of public constants ===== */

/* ===== Public structs and enums ===== */
typedef enum {
    HTTP_RX,
    HTTPS_RX,
    MQTT_RX,
    BLE_SERVER,
    UART_RX,
    I2C_SLAVE,
}   rx_module_t;

typedef enum {
    CMD_START,
    CMD_STOP,
    CMD_STATUS,
    CMD_RESTART,
    CMD_WIFI,
    CMD_BLE,
    CMD_ECHO,
}   command_type_t;

typedef struct {
    rx_module_t     rx_id;
    command_type_t  command; 
}   rx_command_t;

/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: initialize_command_processor
| ------------------------------------------------------------------
|  Description: 
|
|  Parameters:
|       -
|
|  Returns:  int8_t
*-------------------------------------------------------------------*/
int8_t initialize_command_processor();


/*------------------------------------------------------------------
|  Function: command_processor_task
| ------------------------------------------------------------------
|  Description: FreeRTOS task.
|
|  Parameters:
|       - pvParameter: void pointer used as task parameter during
|                      task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void command_processor_task(void *pvParameter);


/* ===== Avoid multiple inclusion ===== */
#endif // __COMMAND_PROCESSOR_H__
