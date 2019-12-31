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
/*------------------------------------------------------------------
|  Enum: rx_module_t
| ------------------------------------------------------------------
|  Description: holds possible modules that can send commands.
|
|  Values:
|       HTTP_RX         - HTTP module
|       HTTPS_RX        - HTTPS module
|       MQTT_RX         - MQTT module
|       BLE_SERVER      - BLE module
|       UART_RX         - UART module
|       I2C_MASTER_MOD  - I2C master module
*-------------------------------------------------------------------*/
typedef enum {
    HTTP_RX,
    HTTPS_RX,
    MQTT_RX,
    BLE_SERVER,
    UART_RX,
    I2C_MASTER_MOD,
}   rx_module_t;

/*------------------------------------------------------------------
|  Enum: command_type_t
| ------------------------------------------------------------------
|  Description: possible commands that can be received.
|
|  Values:
|       CMD_START   - TBD
|       CMD_STOP    - TBD
|       CMD_STATUS  - TBD
|       CMD_RESTART - TBD
|       CMD_WIFI    - toggles WiFi connection (stops BLE if enabled)
|       CMD_BLE     - toggles BLE connection (stops WiFi if enabled)
|       CMD_ECHO    - sends back the same command to the RX module
|                     that sent the command.
|       CMD_INVALID - invalid command
*-------------------------------------------------------------------*/
typedef enum {
    CMD_SLAVE_START_A,
    CMD_SLAVE_START_B,
    CMD_SLAVE_PAUSE,
    CMD_SLAVE_CONTINUE,
    CMD_SLAVE_RESET,
    CMD_SLAVE_STATUS,
    CMD_SLAVE_OK,
    CMD_SLAVE_FAIL,
    CMD_WIFI,
    CMD_BLE,
    CMD_ECHO,
    CMD_DUMMY,
    CMD_INVALID,
}   command_type_t;

/*------------------------------------------------------------------
|  Enum: wireless_state_t
| ------------------------------------------------------------------
|  Description: indicates which wireless connection is currently
|               active.
|
|  Values:
|       WIFI_MODE       - WiFi currently active.
|       BLE_MODE        - BLE currently active.
|       OFFLINE_MODE    - WiFi and BLE currently inactive.
*-------------------------------------------------------------------*/
typedef enum {
    WIFI_MODE,
    BLE_MODE,
    OFFLINE_MODE,
}   wireless_state_t;

/*------------------------------------------------------------------
|  Struct: rx_command_t
| ------------------------------------------------------------------
|  Description: represents a received command.
|
|  Members:
|       rx_id   - module that sent the command
|       command - type of command received
*-------------------------------------------------------------------*/
typedef struct {
    rx_module_t     rx_id;
    command_type_t  command; 
}   rx_command_t;


/* ===== Prototypes of public functions ===== */
/*------------------------------------------------------------------
|  Function: initialize_command_processor
| ------------------------------------------------------------------
|  Description: initializes the command processor module.
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
|  Description: FreeRTOS task that processes the received commands.
|
|  Parameters:
|       - pvParameter: void pointer used as task parameter during
|                      task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void command_processor_task(void *pvParameter);

/*------------------------------------------------------------------
|  Function: str_to_cmd
| ------------------------------------------------------------------
|  Description: translates a string representing the command to the
|               corresponding enum value.
|
|  Parameters:
|       - str_command: string with the command to translate
|
|  Returns:  command_type_t
*-------------------------------------------------------------------*/
command_type_t str_to_cmd(char* str_command);

/*------------------------------------------------------------------
|  Function: translate_command_type
| ------------------------------------------------------------------
|  Description:
|
|  Parameters:
|       -
|
|  Returns:  char*
*-------------------------------------------------------------------*/
char* translate_command_type(command_type_t command);


/* ===== Avoid multiple inclusion ===== */
#endif // __COMMAND_PROCESSOR_H__
