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
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"


/* ===== Macros of public constants ===== */
#define MAX_WIFI_CONNECT_RETRY 10

/* ===== Public structs and enums ===== */
/*------------------------------------------------------------------
|  Struct: mbedtls_connection_handler_t
| ------------------------------------------------------------------
|  Description: 
|
|  Members:
|		entropy 	- 
|		ctr_drbg 	- 
|		cacert 		- 
|		conf 		- 
|		ssl 		- 
|		server_fd 	- 
*-------------------------------------------------------------------*/
typedef struct {
	mbedtls_entropy_context 	entropy;
	mbedtls_ctr_drbg_context 	ctr_drbg;
	mbedtls_x509_crt 			cacert;
	mbedtls_ssl_config 			conf;
	mbedtls_ssl_context 		ssl;
	mbedtls_net_context 		server_fd;
}	mbedtls_connection_handler_t;



/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: initialize_wifi
| ------------------------------------------------------------------
|  Description: it configures WiFi in STATION mode, initializes
|				the WiFi driver and starts WiFi.
|
|  Parameters:
|		-
|
|  Returns:  void
*-------------------------------------------------------------------*/
void initialize_wifi();


/*------------------------------------------------------------------
|  Function: wifi_tx_task
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
void wifi_tx_task(void *pvParameter);


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
|  Function: wifi_rx_cmd_task
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
void wifi_rx_cmd_task(void * pvParameter);

/* ===== Avoid multiple inclusion ===== */
#endif // __WIFI_H__
