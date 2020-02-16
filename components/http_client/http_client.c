/* ===== [http_client.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "http_client.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"


/* ===== Macros of private constants ===== */


/* ===== Declaration of private or external variables ===== */
static const char *TAG  = "HTTP_CLIENT";

/* ===== Prototypes of private functions ===== */


/* ===== Implementations of public functions ===== */
int send_http_request(int socket_handler, struct addrinfo* res, char* http_request)
{
	// ESP_LOGI(TAG, "Sending HTTP request.");

	if(socket_handler < 0) {
		ESP_LOGE(TAG, "Unable to allocate a new socket, not sending to ThingSpeak the received data.");
		return -1;
	}
	// ESP_LOGI(TAG, "Socket allocated, id=%d.", socket_handler);

	// set socket timeout to 1 second (1000000 us)
	struct timeval timeout = {
		.tv_sec = 1,
		.tv_usec = 1000000,
		};

	lwip_setsockopt(socket_handler, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	// connect to the specified server
	int con_result = lwip_connect(socket_handler, res->ai_addr, res->ai_addrlen);
	if(con_result != 0) {
		ESP_LOGE(TAG, "Unable to connect to the target website, not sending to the page the received data.");
		lwip_close_r(socket_handler);
		return -1;
	}
	// ESP_LOGI(TAG, "Connected to the target website.");

	// send the request
	int result = lwip_write(socket_handler, http_request, strlen(http_request));
	if(result < 0) {
		ESP_LOGE(TAG, "Unable to send the HTTP request, not sending to the page the received data.");
		lwip_close_r(socket_handler);
		return -1;
	}
	// ESP_LOGI(TAG, "HTTP request sent:  %s", http_request);
	// ESP_LOGI(TAG, "HTTP request sent.");

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


/* ===== Implementations of private functions ===== */
