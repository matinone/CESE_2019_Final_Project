/* ===== [slave_sim_task.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __SLAVE_SIM_TASK_H__
#define __SLAVE_SIM_TASK_H__

/* ===== Dependencies ===== */
#include "driver/i2c.h"

/* ===== Macros of public constants ===== */

/* ===== Public structs and enums ===== */
typedef enum {
    SLAVE_RESET,
    SLAVE_STOP,
    SLAVE_PROCESS_A,
    SLAVE_PROCESS_B,
    SLAVE_FINISH,
}   slave_machine_state_t;

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

/* ===== Avoid multiple inclusion ===== */
#endif // __SLAVE_SIM_TASK_H__
