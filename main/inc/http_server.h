/* ===== [http_server.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Avoid multiple inclusion ===== */
#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

/* ===== Dependencies ===== */

/* ===== Macros of public constants ===== */
#define WEB_SERVER_TASK_STACK   10000

/* ===== Public structs and enums ===== */

/* ===== Prototypes of public functions ===== */
/*------------------------------------------------------------------
|  Function: http_server
| ------------------------------------------------------------------
|  Description: FreeRTOS task for the HTTP server. It processes
|				all the incoming requests and responds to them.
|
|  Parameters:
|		- pvParameter: void pointer used as task parameter during
|					   task creation.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void http_server(void *pvParameters);


/* ===== Avoid multiple inclusion ===== */
#endif // __HTTP_SERVER_H__