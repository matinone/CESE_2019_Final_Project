/* ===== [slave_sim_task.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2020
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __SLAVE_SIM_TASK_H__
#define __SLAVE_SIM_TASK_H__

/* ===== Dependencies ===== */


/* ===== Macros of public constants ===== */
#define GPIO_OUTPUT_0			4


/* ===== Public structs and enums ===== */
typedef enum {
    SLAVE_IDLE,
    SLAVE_PAUSE,
    SLAVE_PROCESS_A,
    SLAVE_PROCESS_B,
    SLAVE_DONE,
}   slave_machine_state_t;

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

/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: slave_sim_task
| ------------------------------------------------------------------
|  Description: FreeRTOS task.
|
|  Parameters:
|		- pvParameter: void pointer used as task parameter during
|					   task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void slave_sim_task(void *pvParameter);

/*------------------------------------------------------------------
|  Function: translate_slave_machine_state
| ------------------------------------------------------------------
|  Description:
|
|  Parameters:
|       -
|
|  Returns:  char*
*-------------------------------------------------------------------*/
char* translate_slave_machine_state(slave_machine_state_t state);

/* ===== Avoid multiple inclusion ===== */
#endif // __SLAVE_SIM_TASK_H__
