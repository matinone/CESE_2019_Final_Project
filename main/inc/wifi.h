/* ===== [wifi.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __WIFI_H__
#define __WIFI_H__

/* ===== Dependencies ===== */
#include <stdint.h>


/* ===== Macros of public constants ===== */
#define MAX_WIFI_CONNECT_RETRY 10

/* ===== Public structs and enums ===== */


/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: initialize_wifi
| ------------------------------------------------------------------
|  Description: it configures WiFi in STATION mode, initializes
|				the WiFi driver and starts WiFi.
|
|  Parameters:
|		- first_time: flag to be used the first time it is called.
|
|  Returns:  void
*-------------------------------------------------------------------*/
void initialize_wifi(uint8_t first_time);


/*------------------------------------------------------------------
|  Function: stop_wifi
| ------------------------------------------------------------------
|  Description: it disconnects the WiFi and stops the WiFi driver.
|
|  Parameters:
|		-
|
|  Returns:  void
*-------------------------------------------------------------------*/
void stop_wifi();


/* ===== Avoid multiple inclusion ===== */
#endif // __WIFI_H__
