/* ===== [tls_https_client.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __TLS_HTTPS_CLIENT_H__
#define __TLS_HTTPS_CLIENT_H__

/* ===== Dependencies ===== */
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

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
|		server_fd 	- it has a single element of type int 
|					  (the socket handler) named fd
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
|  Function: configure_tls
| ------------------------------------------------------------------
|  Description: 
|
|  Parameters:
|		- 
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int configure_tls(mbedtls_connection_handler_t* mbedtls_handler, char* server, const uint8_t* cert_start, const uint8_t* cert_end);


/*------------------------------------------------------------------
|  Function: tls_send_http_request
| ------------------------------------------------------------------
|  Description: 
|
|  Parameters:
|		- 
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int tls_send_http_request(mbedtls_connection_handler_t* mbedtls_handler, const char* server, const char* port, char* http_request);


/*------------------------------------------------------------------
|  Function: tls_receive_http_response
| ------------------------------------------------------------------
|  Description: 
|
|  Parameters:
|		- 
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int tls_receive_http_response(mbedtls_connection_handler_t* mbedtls_handler, char* recv_buf, char* content_buf, int buf_size);


/*------------------------------------------------------------------
|  Function: tls_clean_up
| ------------------------------------------------------------------
|  Description: 
|
|  Parameters:
|		- 
|
|  Returns: void
*-------------------------------------------------------------------*/
void tls_clean_up(mbedtls_connection_handler_t* mbedtls_handler, int error);


/* ===== Avoid multiple inclusion ===== */
#endif // __TLS_HTTPS_CLIENT_H__
