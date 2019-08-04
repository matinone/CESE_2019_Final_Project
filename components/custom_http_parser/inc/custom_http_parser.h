/* ===== [custom_http_parser.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Avoid multiple inclusion ===== */
#ifndef __CUSTOM_HTTP_PARSER_H__
#define __CUSTOM_HTTP_PARSER_H__

/* ===== Dependencies ===== */
#include <stdint.h>

/* ===== Macros of public constants ===== */


/* ===== Public structs and enums ===== */
/*------------------------------------------------------------------
|  Enum: http_method_t
| ------------------------------------------------------------------
|  Description: possible HTTP methods.
|
|  Values:
|       UNSUPPORTED - unsupported HTTP method
|       GET         - HTTP GET
|       POST        - HTTP POST
*-------------------------------------------------------------------*/
typedef enum {
    UNSUPPORTED, 
    GET, 
    POST
}   http_method_t;


/*------------------------------------------------------------------
|  Struct: http_header_t
| ------------------------------------------------------------------
|  Description: represents a HTTP header.
|
|  Members:
|       name    - header name
|       value   - header value
|       next    - pointer to the next header in a list of headers
*-------------------------------------------------------------------*/
struct http_header_t{
    char*                   name;
    char*                   value;
    struct http_header_t*   next;
};

typedef struct http_header_t http_header_t;


/*------------------------------------------------------------------
|  Struct: http_request_t
| ------------------------------------------------------------------
|  Description: represents a HTTP request.
|
|  Members:
|       method      - HTTP method in the request
|       resource    - resource requested to the server
|       version     - HTTP version
|       headers     - HTTP headers in the request
|       body        - body of the reqquest
*-------------------------------------------------------------------*/
typedef struct {
    http_method_t   method;
    char*           resource;
    char*           version;
    http_header_t*  headers;
    char*           body;
} http_request_t;

/* ===== Prototypes of public functions ===== */
/*------------------------------------------------------------------
|  Function: parse_http_request
| ------------------------------------------------------------------
|  Description: it parses a string representing a HTTP request and
|               creates a structure with the parsed content.
|
|  Parameters:
|       - raw: string with the raw HTTP request.
|       - raw_len: length of the raw HTTP request.
|
|  Returns: http_request_t* - pointer to the parsed HTTP request.
*-------------------------------------------------------------------*/
http_request_t* parse_http_request(const char* raw, uint16_t raw_len);

/*------------------------------------------------------------------
|  Function: free_header
| ------------------------------------------------------------------
|  Description: it frees the memory of a HTTP header.
|
|  Parameters:
|       - header: pointer to the header to free.
|
|  Returns: void
*-------------------------------------------------------------------*/
void free_header(http_header_t* header);

/*------------------------------------------------------------------
|  Function: free_request
| ------------------------------------------------------------------
|  Description: it frees the memory of a HTTP request.
|
|  Parameters:
|       - request: pointer to the request to free.
|
|  Returns: void
*-------------------------------------------------------------------*/
void free_request(http_request_t* request);


/* ===== Avoid multiple inclusion ===== */
#endif // __CUSTOM_HTTP_PARSER_H__
