/* ===== [wifi.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "wifi.h"
#include "thingspeak_http_request.h"
#include "http_client.h"
#include "tls_https_client.h"
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

#define WEB_SERVER "www.thingspeak.com"
#define WEB_PORT "443"

#define RX_BUFFER_SIZE 128

/* ===== Declaration of private or external variables ===== */
extern QueueHandle_t queue_command_processor_rx;
extern EventGroupHandle_t wifi_event_group;

// the PEM file was extracted from the output of this command:
// openssl s_client -showcerts -connect www.thingspeak.com:443 </dev/null
// the CA root cert is the last certificate given in the chain of certs
extern const uint8_t thingspeak_https_cert_start[] asm("_binary_thingspeak_https_certificate_pem_start");
extern const uint8_t thingspeak_https_cert_end[]   asm("_binary_thingspeak_https_certificate_pem_end");

static const int CONNECTED_BIT = BIT0;
static const char *TAG = "TLS_HTTPS_TASK";

QueueHandle_t queue_tls_https_tx;


/* ===== Prototypes of private functions ===== */


/* ===== Implementations of public functions ===== */
void wifi_secure_tx_task(void *pvParameter)
{
	char recv_buf[RX_BUFFER_SIZE];
	char content_buf[RX_BUFFER_SIZE];
	int ret;

	// create a queue capable of containing 5 uint8_t values
    queue_tls_https_tx = xQueueCreate(5, sizeof(uint8_t));
    if (queue_tls_https_tx == NULL)
    {
        printf("Could not create queue_tls_https_tx.\n");
    }

	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("WiFi successfully connected.\n\n");

	mbedtls_connection_handler_t mbedtls_handler;
	ret = configure_tls(&mbedtls_handler, WEB_SERVER, thingspeak_https_cert_start, thingspeak_https_cert_end);
	if (ret != 0)
	{
		abort();
	}

	BaseType_t xStatus;
	uint8_t queue_rcv_value;
	char request_buffer[strlen(HTTP_REQUEST_WRITE)];

	while(1) {
		// always wait for connection
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

		// read data from the queue
		xStatus = xQueueReceive(queue_tls_https_tx, &queue_rcv_value,  20 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			ESP_LOGI(TAG, "Received from Command Processor TASK: %d\n", queue_rcv_value);
			sprintf(request_buffer, HTTP_REQUEST_WRITE, queue_rcv_value);

			ret = tls_send_http_request(&mbedtls_handler, WEB_SERVER, WEB_PORT, request_buffer);
			if (ret != 0)
			{
				tls_clean_up(&mbedtls_handler, ret);
				continue;
			}

			ESP_LOGI(TAG, "Receiving HTTP response.\n");
			content_buf[0] = '\0';
			int flag_rsp_ok = tls_receive_http_response(&mbedtls_handler,recv_buf, content_buf, RX_BUFFER_SIZE);

			tls_clean_up(&mbedtls_handler, ret);

			if (flag_rsp_ok == 1)
			{
				printf("HTTP response status OK.\n");
				printf("Response Content: %s\n", content_buf);
			}
			else 
			{
				printf("HTTP response status NOT OK.\n");
			}

			putchar('\n'); // JSON output doesn't have a newline at end
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void wifi_secure_rx_cmd_task(void * pvParameter)
{
	// wait for connection
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	int ret;
	char recv_buf[RX_BUFFER_SIZE];
	char content_buf[RX_BUFFER_SIZE];
	char * pch;
	BaseType_t xStatus;

	// command to send to the command processor
	rx_command_t tls_https_command;
    tls_https_command.rx_id = HTTPS_RX;

	mbedtls_connection_handler_t mbedtls_handler;
	ret = configure_tls(&mbedtls_handler, WEB_SERVER, thingspeak_https_cert_start, thingspeak_https_cert_end);
	if (ret != 0)
	{
		abort();
	}

	while (1)
	{
		// always wait for connection
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
		
		printf("\nChecking if there is any new command to execute.\n");

		ret = tls_send_http_request(&mbedtls_handler, WEB_SERVER, WEB_PORT, HTTP_REQUEST_READ_CMD);
		if (ret != 0)
		{
			tls_clean_up(&mbedtls_handler, ret);
			continue;
		}
		
		ESP_LOGI(TAG, "Receiving HTTP response.\n");
		content_buf[0] = '\0';
		int flag_rsp_ok = tls_receive_http_response(&mbedtls_handler, recv_buf, content_buf, RX_BUFFER_SIZE);

		tls_clean_up(&mbedtls_handler, ret);

		if (flag_rsp_ok == 1)
		{
			printf("HTTP response status OK.\n");
			// printf("Response Content: %s\n", content_buf);

			pch = strstr(content_buf, "CMD_");
			if (pch != NULL)
			{
				printf("Received new command: %s\n", content_buf);

				tls_https_command.command = str_to_cmd(content_buf);
				xStatus = xQueueSendToBack(queue_command_processor_rx, &tls_https_command, 1000 / portTICK_RATE_MS);
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

		putchar('\n'); // JSON output doesn't have a newline at end

		vTaskDelay(COMMAND_RX_CHECK_PERIOD / portTICK_RATE_MS);
	}
}


/* ===== Implementations of private functions ===== */
