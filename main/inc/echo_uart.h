/* ===== [echo_uart.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Avoid multiple inclusion ===== */
#ifndef __ECHO_UART_H__
#define __ECHO_UART_H__

/* ===== Dependencies ===== */

/* ===== Macros of public constants ===== */

/* ===== Public structs and enums ===== */

/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: initialize_uart
| ------------------------------------------------------------------
|  Description: configures UART_NUM_0 with 115200 baud rate and 
|				other default values.
|
|  Parameters:
|		-
|
|  Returns:  void
*-------------------------------------------------------------------*/
void initialize_uart();


/*------------------------------------------------------------------
|  Function: echo_task
| ------------------------------------------------------------------
|  Description: FreeRTOS task. It checks every 100ms if
|				there is new data to be received through UART and
|				it passes that data to the I2C Slave task.
|
|  Parameters:
|		- pvParameter: void pointer used as task parameter during
|					   task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void echo_task(void *pvParameter);


/* ===== Avoid multiple inclusion ===== */
#endif // __ECHO_UART_H__
