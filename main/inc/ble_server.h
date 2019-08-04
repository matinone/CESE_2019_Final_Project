/* ===== [ble_server.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Avoid multiple inclusion ===== */
#ifndef __BLE_SERVER_H__
#define __BLE_SERVER_H__

/* ===== Dependencies ===== */
#include <stdint.h>

/* ===== Macros of public constants ===== */

/* ===== Public structs and enums ===== */

/* ===== Prototypes of public functions ===== */
/*------------------------------------------------------------------
|  Function: start_ble_server
| ------------------------------------------------------------------
|  Description: it starts the BLE server.
|
|  Parameters:
|
|  Returns:	int8_t
*-------------------------------------------------------------------*/
int8_t start_ble_server();

/*------------------------------------------------------------------
|  Function: stop_ble_server
| ------------------------------------------------------------------
|  Description: it stops the BLE server.
|
|  Parameters:
|
|  Returns:	int8_t
*-------------------------------------------------------------------*/
int8_t stop_ble_server();


/* ===== Avoid multiple inclusion ===== */
#endif // __BLE_SERVER_H__