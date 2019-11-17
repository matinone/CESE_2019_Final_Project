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
|  Description: sends a HTTP request to a server.
|
|  Parameters:
|		- socket_handler: handler of the socket used to send the
|       request.
|       - res: target server for the HTTP request.
|       - http_request: the specific request to send.
|
|  Returns: int
|			0: OK
|          -1: ERROR, unable to send the HTTP request
*-------------------------------------------------------------------*/
int send_http_request(int socket_handler, struct addrinfo* res, char* http_request);


/*------------------------------------------------------------------
|  Function: receive_http_response
| ------------------------------------------------------------------
|  Description: gets the HTTP response after sending a HTTP request
|
|  Parameters:
|		- socket_handler: handler of the socket used to receive the
|       response.
|       - recv_buf: buffer to receive the whole HTTP response.
|       - content_buf: buffer for the content section of the 
|       response.
|       - buf_size: size of the recv_buf.
|
|  Returns: int
|			0           : OK
|           Other value : ERROR
*-------------------------------------------------------------------*/
int receive_http_response(int socket_handler, char* recv_buf, char* content_buf, int buf_size);


/* ===== Avoid multiple inclusion ===== */
#endif // __HTTP_CLIENT_H__
