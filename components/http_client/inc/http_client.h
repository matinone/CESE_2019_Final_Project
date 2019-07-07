#include "lwip/netdb.h"
#include "lwip/sockets.h"

int send_http_request(int socket_handler, struct addrinfo* res, char* http_request);
int receive_http_response(int socket_handler, char* recv_buf, char* content_buf, int buf_size);
