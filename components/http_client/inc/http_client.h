/* ===== [http_client.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

/* ===== Dependencies ===== */
#include "lwip/netdb.h"
#include "lwip/sockets.h"

/* ===== Public structs and enums ===== */


/* ===== Prototypes of public functions ===== */

/*------------------------------------------------------------------
|  Function: send_http_request
| ------------------------------------------------------------------
|  Description: 
|
|  Parameters:
|		- 
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int send_http_request(int socket_handler, struct addrinfo* res, char* http_request);


/*------------------------------------------------------------------
|  Function: receive_http_response
| ------------------------------------------------------------------
|  Description: 
|
|  Parameters:
|		- 
|
|  Returns: int
|			0: OK
*-------------------------------------------------------------------*/
int receive_http_response(int socket_handler, char* recv_buf, char* content_buf, int buf_size);


/* ===== Avoid multiple inclusion ===== */
#endif // __HTTP_CLIENT_H__
