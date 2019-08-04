/* ===== [custom_http_parser.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "custom_http_parser.h"

#include <stdlib.h>
#include <string.h>

/* ===== Macros of private constants ===== */

/* ===== Declaration of private or external variables ===== */

/* ===== Prototypes of private functions ===== */

/* ===== Implementations of public functions ===== */
http_request_t* parse_http_request(const char* raw, uint16_t raw_len) 
{
    const char* initial_raw;
    initial_raw = raw;

    http_request_t* req = NULL;
    req = malloc(sizeof(http_request_t));
    if (!req) 
    {
        return NULL;
    }
    memset(req, 0, sizeof(http_request_t));

    // find HTTP method
    size_t meth_len = strcspn(raw, " ");
    if (memcmp(raw, "GET", strlen("GET")) == 0) 
    {
        req->method = GET;
    } 
    else if (memcmp(raw, "POST", strlen("POST")) == 0) 
    {
        req->method = POST;
    } 
    else 
    {
        req->method = UNSUPPORTED;
    }
    raw += meth_len + 1; // move past <SP>

    // find request URI
    size_t resource_len = strcspn(raw, " ");
    req->resource = malloc(resource_len + 1);
    if (!req->resource) 
    {
        free_request(req);
        return NULL;
    }
    memcpy(req->resource, raw, resource_len);
    req->resource[resource_len] = '\0';
    raw += resource_len + 1; // move past <SP>

    // find HTTP version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = malloc(ver_len + 1);
    if (!req->version) 
    {
        free_request(req);
        return NULL;
    }
    memcpy(req->version, raw, ver_len);
    req->version[ver_len] = '\0';
    raw += ver_len + 2; // move past <CR><LF>

    // find all headers
    http_header_t* header = NULL;
    http_header_t* last = NULL;
    while (raw[0]!='\r' || raw[1]!='\n')
    {
        last = header;
        header = malloc(sizeof(http_header_t));
        if (!header) 
        {
            free_request(req);
            return NULL;
        }

        // find header name
        size_t name_len = strcspn(raw, ":");
        header->name = malloc(name_len + 1);
        if (!header->name) 
        {
            free(header);           // free current header
            req->headers = last;    // free the previous headers
            free_request(req);
            return NULL;
        }
        memcpy(header->name, raw, name_len);
        header->name[name_len] = '\0';
        raw += name_len + 1; // move past :
        while (*raw == ' ') 
        {
            raw++;
        }

        // find header value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = malloc(value_len + 1);
        if (!header->value) 
        {
            free(header);           // free current header
            req->headers = last;    // free the previous headers
            free_request(req);
            return NULL;
        }
        memcpy(header->value, raw, value_len);
        header->value[value_len] = '\0';
        raw += value_len + 2; // move past <CR><LF>

        // parse next header
        header->next = last;
    }
    req->headers = header;
    raw += 2; // move past <CR><LF>

    // cant use strlen(raw) because it is not null terminated
    size_t body_len = initial_raw + raw_len - raw;
    // size_t body_len = strlen(raw);

    req->body = malloc(body_len + 1);
    if (!req->body) 
    {
        free_request(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);
    req->body[body_len] = '\0';

    return req;
}


void free_header(http_header_t* header) {
    if (header) 
    {
        free(header->name);
        free(header->value);
        free_header(header->next);
        free(header);
    }
}


void free_request(http_request_t* req) {
    if (req)
    {
        free(req->resource);
        free(req->version);
        free_header(req->headers);
        free(req->body);
        free(req);
    }
}

/* ===== Implementations of private functions ===== */
