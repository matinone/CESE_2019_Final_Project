#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

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


int configure_tls(mbedtls_connection_handler_t* mbedtls_handler, char* server, const uint8_t* cert_start, const uint8_t* cert_end);
int tls_send_http_request(mbedtls_connection_handler_t* mbedtls_handler, const char* server, const char* port, char* http_request);
int tls_receive_http_response(mbedtls_connection_handler_t* mbedtls_handler, char* recv_buf, char* content_buf, int buf_size);
