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
|  Description: contains all the information necessary for the TLS
|				connection.
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
|  Description: configures all the parameters of the TLS connection.
|
|  Parameters:
|		- mbedtls_handler: handler with all the information of the
|		TLS connection.
|		- server: hostname for the TLS session.
|		- cert_start: pointer to the CA certificate initial memory
|		address
|		- cert_end: pointer to the CA certificate last memory
|		address
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int configure_tls(mbedtls_connection_handler_t* mbedtls_handler, char* server, const uint8_t* cert_start, const uint8_t* cert_end);


/*------------------------------------------------------------------
|  Function: tls_send_http_request
| ------------------------------------------------------------------
|  Description: connects to the server, performs the SSL/TLS
|				handshake, and sends the HTTP request.
|
|  Parameters:
|		- mbedtls_handler: handler with all the information of the
|		TLS connection.
|		- server: server to connect to.
|		- port: port to connect to.
|		- http_request: HTTP request to send using the TLS session.
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int tls_send_http_request(mbedtls_connection_handler_t* mbedtls_handler, const char* server, const char* port, char* http_request);


/*------------------------------------------------------------------
|  Function: tls_receive_http_response
| ------------------------------------------------------------------
|  Description: gets the HTTP response after sending a HTTP request,
|				through TLS.
|
|  Parameters:
|		- mbedtls_handler: handler with all the information of the
|		TLS connection.
|		- recv_buf: buffer to receive the whole HTTP response.
|       - content_buf: buffer for the content section of the 
|       response.
|       - buf_size: size of the recv_buf.
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int tls_receive_http_response(mbedtls_connection_handler_t* mbedtls_handler, char* recv_buf, char* content_buf, int buf_size);


/*------------------------------------------------------------------
|  Function: tls_clean_up
| ------------------------------------------------------------------
|  Description: closes the TLS session and frees up the memory.
|
|  Parameters:
|		- mbedtls_handler: handler with all the information of the
|		TLS connection.
|		- error: contains the error code (in case of error)
|
|  Returns: void
*-------------------------------------------------------------------*/
void tls_clean_up(mbedtls_connection_handler_t* mbedtls_handler, int error);


/* ===== Avoid multiple inclusion ===== */
#endif // __TLS_HTTPS_CLIENT_H__
