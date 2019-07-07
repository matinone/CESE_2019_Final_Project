#include "http_client.h"

#include <stdio.h>
#include <string.h>

int send_http_request(int socket_handler, struct addrinfo* res, char* http_request)
{
	printf("Sending HTTP request.\n");

	if(socket_handler < 0) {
		printf("Unable to allocate a new socket, not sending to ThingSpeak the received data.\n");
		return -1;
	}
	// printf("Socket allocated, id=%d.\n", socket_handler);

	// set socket timeout to 1 second (1000000 us)
	struct timeval timeout = {
		.tv_sec = 1,
		.tv_usec = 1000000,
		};

	lwip_setsockopt(socket_handler, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	// connect to the specified server
	int con_result = lwip_connect(socket_handler, res->ai_addr, res->ai_addrlen);
	if(con_result != 0) {
		printf("Unable to connect to the target website, not sending to ThingSpeak the received data.\n");
		lwip_close(socket_handler);
		return -1;
	}
	// printf("Connected to the target website.\n");

	// send the request
	int result = lwip_write(socket_handler, http_request, strlen(http_request));
	if(result < 0) {
		printf("Unable to send the HTTP request, not sending to ThingSpeak the received data.\n");
		lwip_close(socket_handler);
		return -1;
	}
	// printf("HTTP request sent:  %s\n", http_request);
	printf("HTTP request sent.\n");

	return 0;
}

int receive_http_response(int socket_handler, char* recv_buf, char* content_buf, int buf_size)
{	
	int r;
	int flag_content = 0;
	int flag_rsp_ok = 0;
	char * pch;

	do {
		bzero(recv_buf, buf_size);
		r = lwip_read(socket_handler, recv_buf, buf_size - 1);

		if (strstr (recv_buf,"Status") != NULL && strstr (recv_buf,"200 OK") != NULL)
		{
			flag_rsp_ok = 1;
		}

		// check if the CONTENT of the response arrived
		pch = strstr(recv_buf, "\n\r\n");
		if (pch != NULL || flag_content == 1)
		{
			if (pch != NULL)
			{
				strcat(content_buf, pch+3);	// pch + 3 to ignore the LF+CR+LF
			}
			else
			{
				strcat(content_buf, recv_buf);
			}
			
			flag_content = 1;
		}

	} while(r > 0);

	return flag_rsp_ok;
}