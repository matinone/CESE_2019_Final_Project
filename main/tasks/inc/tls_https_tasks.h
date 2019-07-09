/* ===== [tls_https_tasks.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __TLS_HTTPS_TASKS_H__
#define __TLS_HTTPS_TASKS_H__

/* ===== Dependencies ===== */


/* ===== Macros of public constants ===== */


/* ===== Public structs and enums ===== */


/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: wifi_secure_tx_task
| ------------------------------------------------------------------
|  Description: FreeRTOS task. It waits for the WiFi connection to
|				be ready, resolves the IP of the target website
|				and waits for data to be received from I2C master
|				task. Once data arrives, it creates a TCP socket
|				and sends and HTTP request to the website to
|				publish the received data.
|
|  Parameters:
|		- pvParameter: void pointer used as task parameter during
|					   task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void wifi_secure_tx_task(void *pvParameter);


/*------------------------------------------------------------------
|  Function: wifi_secure_rx_cmd_task
| ------------------------------------------------------------------
|  Description: FreeRTOS task. It waits for the target website IP
|				to be resolved and then waits for commands to be 
|				received from ThingSpeak server.
|
|  Parameters:
|		- pvParameter: void pointer used as task parameter during
|					   task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void wifi_secure_rx_cmd_task(void * pvParameter);


/* ===== Avoid multiple inclusion ===== */
#endif // __TLS_HTTPS_TASKS_H__
