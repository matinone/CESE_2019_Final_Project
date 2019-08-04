#include <stdint.h>

typedef enum {
    UNSUPPORTED, 
    GET, 
    POST
}   http_method_t;

struct http_header_t{
    char*                   name;
    char*                   value;
    struct http_header_t*   next;
};

typedef struct http_header_t http_header_t;

typedef struct {
    http_method_t   method;
    char*           resource;
    char*           version;
    http_header_t*  headers;
    char*           body;
} http_request_t;


http_request_t* parse_http_request(const char* raw, uint16_t raw_len);
void free_header(http_header_t* header);
void free_request(http_request_t* request);