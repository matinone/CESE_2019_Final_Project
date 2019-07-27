/* ===== [http_tasks.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "http_tasks.h"
#include "wifi.h"
#include "thingspeak_http_request.h"
#include "http_client.h"
#include "command_processor.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"


/* ===== Macros of private constants ===== */
#define COMMAND_RX_CHECK_PERIOD 15000
#define RX_BUFFER_SIZE 128

/* ===== Declaration of private or external variables ===== */
extern EventGroupHandle_t wifi_event_group;
extern QueueHandle_t queue_command_processor_rx;

static const int CONNECTED_BIT = BIT0;
static const char *TAG = "HTTP_TASK";

// queue to pass the IP resolution from wifi_tx_task to wifi_rx_cmd_task
QueueHandle_t queue_wifi_tx_to_rx;
QueueHandle_t queue_http_tx;


/* ===== Prototypes of private functions ===== */


/* ===== Implementations of public functions ===== */
void wifi_tx_task(void *pvParameter)
{
	// create a queue capable of containing 5 uint8_t values
    queue_http_tx = xQueueCreate(5, sizeof(uint8_t));
    if (queue_http_tx == NULL)
    {
        printf("Could not create queue_http_tx.\n");
    }

	// create a queue capable of containing a single pointer to struct addrinfo
	queue_wifi_tx_to_rx = xQueueCreate(1, sizeof(struct addrinfo *));
	if (queue_wifi_tx_to_rx == NULL)
	{
		printf("Could not create wifi_tx_to_rx QUEUE.\n");
	}

	// wait for connection
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("WiFi successfully connected.\n\n");
	
	// print the local IP address
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
	printf("\n");
	
	// define connection parameters
	const struct addrinfo hints = {
		.ai_family = AF_INET,		// IPv4
		.ai_socktype = SOCK_STREAM,	// TCP
	};

	// address info struct and receive buffers
	struct addrinfo *res;
	char recv_buf[RX_BUFFER_SIZE];
	char content_buf[RX_BUFFER_SIZE];
	
	// resolve the IP of the target website
	int result = getaddrinfo(CONFIG_HTTP_WEBSITE, "80", &hints, &res);
	if((result != 0) || (res == NULL)) {
		printf("Unable to resolve IP for target website %s\n", CONFIG_HTTP_WEBSITE);
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}
	printf("Target website's IP resolved for %s\n", CONFIG_HTTP_WEBSITE);

	BaseType_t xStatus;
	xStatus = xQueueSendToBack(queue_wifi_tx_to_rx, &res, 0);
	if (xStatus != pdPASS)
	{
		printf("Could not send the IP address resolve information to the queue.\n");
	}

	uint8_t queue_rcv_value;
	char request_buffer[strlen(HTTP_REQUEST_WRITE)];

	while (1)
	{
		// always wait for connection
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
		
		// Read data from the queue
		xStatus = xQueueReceive(queue_http_tx, &queue_rcv_value,  20 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			ESP_LOGI(TAG, "Received from Command Processor TASK: %d\n", queue_rcv_value);
			sprintf(request_buffer, HTTP_REQUEST_WRITE, queue_rcv_value);
			
			// create a new socket
			int s = lwip_socket(res->ai_family, res->ai_socktype, 0);
			int request_status = send_http_request(s, res, request_buffer);
			if (request_status < 0)
			{
				continue;
			}
			
			printf("Receiving HTTP response.\n");
			int flag_rsp_ok = 0;
			content_buf[0] = '\0';
			
			flag_rsp_ok = receive_http_response(s, recv_buf, content_buf, RX_BUFFER_SIZE);

			// close socket after receiving the response
			lwip_close(s);

			if (flag_rsp_ok == 1)
			{
				printf("HTTP response status OK.\n");
				printf("Response Content: %s\n", content_buf);

				// pch = strtok (content_buf,"\n,");
				// while (pch != NULL && strcmp(pch, "\r") != 0)
				// {
				// 	printf ("Token: %s\n", pch);
				//  	pch = strtok (NULL, "\n,");
				// }
			}
			else 
			{
				printf("HTTP response status NOT OK.\n");
			}

		}   // xStatus == pdPASS
		else 
		{
			// printf("Nothing received from ECHO Task.\n");
		}

		vTaskDelay(1000 / portTICK_RATE_MS);
	}   // while(1)
}


void wifi_rx_cmd_task(void * pvParameter)
{
	// wait for connection
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	BaseType_t xStatus;
	// address info struct and receive buffers
	struct addrinfo *res;
	char recv_buf[RX_BUFFER_SIZE];
	char content_buf[RX_BUFFER_SIZE];
	char * pch;
	// command to send to the command processor
	rx_command_t http_command;
    http_command.rx_id = HTTP_RX;

	if (xQueueReceive(queue_wifi_tx_to_rx, &res, portMAX_DELAY) != pdTRUE)
	{
		printf("Error receiving target website IP resolution\n");
	}

	// wait some initial time
	vTaskDelay(5000 / portTICK_RATE_MS);

	while (1)
	{
		// always wait for connection
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

		// printf("Free HEAP: %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
		// heap_caps_get_minimum_free_size

		printf("\nChecking if there is any new command to execute.\n");

		// create a new socket
		int s = socket(res->ai_family, res->ai_socktype, 0);
		int request_status = send_http_request(s, res, HTTP_REQUEST_READ_CMD);
		if (request_status < 0)
		{
			vTaskDelay(COMMAND_RX_CHECK_PERIOD / portTICK_RATE_MS);
			continue;
		}
		
		printf("Receiving HTTP response.\n");
		int flag_rsp_ok;
		content_buf[0] = '\0';
		flag_rsp_ok = receive_http_response(s, recv_buf, content_buf, RX_BUFFER_SIZE);

		// close socket after receiving the response
		lwip_close(s);

		if (flag_rsp_ok == 1)
		{
			printf("HTTP response status OK.\n");
			// printf("Response Content: %s\n", content_buf);

			pch = strstr(content_buf, "CMD_");
			if (pch != NULL)
			{
				printf("Received new command: %s\n", content_buf);
				
				http_command.command = str_to_cmd(content_buf);
				xStatus = xQueueSendToBack(queue_command_processor_rx, &http_command, 1000 / portTICK_RATE_MS);
	            if (xStatus != pdPASS)
	            {
	                printf("Could not send the data to the queue.\n");
	            }
			}
			else
			{
				printf("No new commands.\n");
			}
		}
		else 
		{
			printf("HTTP response status NOT OK.\n");
		}
		vTaskDelay(COMMAND_RX_CHECK_PERIOD / portTICK_RATE_MS);
	}
}


/* ===== Implementations of private functions ===== */
