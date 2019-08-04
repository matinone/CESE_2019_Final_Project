/* ===== [nvs_storage.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Avoid multiple inclusion ===== */
#ifndef __NVS_STORAGE_H__
#define __NVS_STORAGE_H__

/* ===== Dependencies ===== */
#include <stdint.h>
#include "esp_err.h"

/* ===== Macros of public constants ===== */
#define INCLUDE_NVS_STORAGE 	1
#define WIFI_SSID_NVS_KEY 		"wifi_ssid"
#define WIFI_PASSWORD_NVS_KEY 	"wifi_password"

/* ===== Public structs and enums ===== */


/* ===== Prototypes of public functions ===== */
/*------------------------------------------------------------------
|  Function: init_nvs_storage
| ------------------------------------------------------------------
|  Description: initializes NVS (Non Volatile Storage)
|
|  Parameters:
|		- storage: if equal to INCLUDE_NVS_STORAGE, the function
|		also opens the storage partition.
|
|  Returns:  esp_err_t
*-------------------------------------------------------------------*/
esp_err_t init_nvs_storage(uint8_t storage);

/*------------------------------------------------------------------
|  Function: get_nvs_string_value
| ------------------------------------------------------------------
|  Description: returns the string value from the NVS partition 
|				associated to the string_key.
|
|  Parameters:
|		- string_key: key to use when accessing the NVS partition.
|
|  Returns:	char* - string associated to string_key
*-------------------------------------------------------------------*/
char* get_nvs_string_value(char* string_key);

/*------------------------------------------------------------------
|  Function: set_nvs_string_value
| ------------------------------------------------------------------
|  Description: stores in the NVS storage partition a value 
|				associated to a key.
|
|  Parameters:
|		- string_key: 	key to use.
|		- string_value:	value to store.
|
|  Returns:  esp_err_t
*-------------------------------------------------------------------*/
esp_err_t set_nvs_string_value(char* string_key, char* string_value);

/* ===== Avoid multiple inclusion ===== */
#endif // __NVS_STORAGE_H__
