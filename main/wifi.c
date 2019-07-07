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

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "mbedtls/platform.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

/* ===== Macros of private constants ===== */
#define COMMAND_RX_CHECK_PERIOD 15000

#define WEB_SERVER "www.thingspeak.com"
#define WEB_PORT "443"

#define RX_BUFFER_SIZE 128

/* ===== Declaration of private or external variables ===== */
extern QueueHandle_t queue_i2c_to_wifi;
extern EventGroupHandle_t wifi_event_group;

// the PEM file was extracted from the output of this command:
// openssl s_client -showcerts -connect www.thingspeak.com:443 </dev/null
// the CA root cert is the last certificate given in the chain of certs
extern const uint8_t server_root_cert_pem_start[] asm("_binary_openssl_certificate_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_openssl_certificate_pem_end");

static int connect_retry_num = 0;
const int CONNECTED_BIT = BIT0;
static const char *TAG = "WIFI_TASK";

// queue to pass the IP resolution from wifi_tx_task to wifi_rx_cmd_task
QueueHandle_t queue_wifi_tx_to_rx;

/* ===== Prototypes of private functions ===== */
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
static int configure_tls(mbedtls_connection_handler_t* mbedtls_handler, const uint8_t* cert_start, const uint8_t* cert_end);
static int tls_send_http_request(mbedtls_connection_handler_t* mbedtls_handler, const char* server, const char* port, char* http_request);
static int tls_receive_http_response(mbedtls_connection_handler_t* mbedtls_handler, char* recv_buf, char* content_buf, int buf_size);


/* ===== Implementations of public functions ===== */
void initialize_wifi()
{
	// create a queue capable of containing a single pointer to struct addrinfo
	queue_wifi_tx_to_rx = xQueueCreate(1, sizeof(struct addrinfo *));
	if (queue_wifi_tx_to_rx == NULL)
	{
		printf("Could not create wifi_tx_to_rx QUEUE.\n");
	}

	// create the event group to handle wifi events
	wifi_event_group = xEventGroupCreate();
	
	// initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	
	// initialize the wifi stack in STAtion mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	// configure the wifi connection and start the interface
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_WIFI_SSID,
			.password = CONFIG_WIFI_PASSWORD,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("Connecting to %s.", CONFIG_WIFI_SSID);
}


void wifi_tx_task(void *pvParameter)
{
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
	int result = getaddrinfo(CONFIG_WEBSITE, "80", &hints, &res);
	if((result != 0) || (res == NULL)) {
		printf("Unable to resolve IP for target website %s\n", CONFIG_WEBSITE);
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}
	printf("Target website's IP resolved for target website %s\n", CONFIG_WEBSITE);

	BaseType_t xStatus;
	xStatus = xQueueSendToBack(queue_wifi_tx_to_rx, &res, 0);
	if (xStatus != pdPASS)
	{
		printf("Could not send the IP address resolve information to the queue.\n");
	}

	uint8_t queue_rcv_value;
	char request_buffer[strlen(HTTP_REQUEST_WRITE)];
	// char * pch;

	while (1)
	{
		// Read data from the queue
		xStatus = xQueueReceive( queue_i2c_to_wifi, &queue_rcv_value,  20 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			ESP_LOGI(TAG, "Received from I2C MASTER TASK: %c\n", queue_rcv_value);
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


void wifi_secure_tx_task(void *pvParameter)
{
	char recv_buf[RX_BUFFER_SIZE];
	char content_buf[RX_BUFFER_SIZE];
	int ret;

	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("WiFi successfully connected.\n\n");

	mbedtls_connection_handler_t mbedtls_handler;
	ret = configure_tls(&mbedtls_handler, server_root_cert_pem_start, server_root_cert_pem_end);
	if (ret != 0)
	{
		abort();
	}

	BaseType_t xStatus;
	uint8_t queue_rcv_value;
	char request_buffer[strlen(HTTP_REQUEST_WRITE)];

	while(1) {
		// Read data from the queue
		xStatus = xQueueReceive( queue_i2c_to_wifi, &queue_rcv_value,  20 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			ESP_LOGI(TAG, "Received from I2C MASTER TASK: %c\n", queue_rcv_value);
			sprintf(request_buffer, HTTP_REQUEST_WRITE, queue_rcv_value);

			ret = tls_send_http_request(&mbedtls_handler, WEB_SERVER, WEB_PORT, request_buffer);
			if (ret != 0)
			{
				goto exit;
			}

			ESP_LOGI(TAG, "Receiving HTTP response.\n");
			content_buf[0] = '\0';
			int flag_rsp_ok = tls_receive_http_response(&mbedtls_handler,recv_buf, content_buf, RX_BUFFER_SIZE);

			mbedtls_ssl_close_notify(&mbedtls_handler.ssl);

			if (flag_rsp_ok == 1)
			{
				printf("HTTP response status OK.\n");
				printf("Response Content: %s\n", content_buf);
			}
			else 
			{
				printf("HTTP response status NOT OK.\n");
			}

		exit:
			mbedtls_ssl_session_reset(&mbedtls_handler.ssl);
			mbedtls_net_free(&mbedtls_handler.server_fd);

			if(ret != 0)
			{
				mbedtls_strerror(ret, recv_buf, 100);
				ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, recv_buf);
			}

			putchar('\n'); // JSON output doesn't have a newline at end
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void wifi_rx_cmd_task(void * pvParameter)
{
	// wait for connection
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	// address info struct and receive buffers
	struct addrinfo *res;
	char recv_buf[RX_BUFFER_SIZE];
	char content_buf[RX_BUFFER_SIZE];
	char * pch;

	if (xQueueReceive(queue_wifi_tx_to_rx, &res, portMAX_DELAY) != pdTRUE)
	{
		printf("Error receiving target website IP resolution\n");
	}

	// wait some initial time
	vTaskDelay(5000 / portTICK_RATE_MS);

	while (1)
	{
		printf("\nChecking if there is any new command to execute.\n");

		// create a new socket
		int s = socket(res->ai_family, res->ai_socktype, 0);
		int request_status = send_http_request(s, res, HTTP_REQUEST_READ_CMD);
		if (request_status < 0)
		{
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


void wifi_secure_rx_cmd_task(void * pvParameter)
{
	// wait for connection
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	int ret;
	char recv_buf[RX_BUFFER_SIZE];
	char content_buf[RX_BUFFER_SIZE];
	char * pch;

	mbedtls_connection_handler_t mbedtls_handler;
	ret = configure_tls(&mbedtls_handler, server_root_cert_pem_start, server_root_cert_pem_end);
	if (ret != 0)
	{
		abort();
	}

	while (1)
	{
		printf("\nChecking if there is any new command to execute.\n");

		ret = tls_send_http_request(&mbedtls_handler, WEB_SERVER, WEB_PORT, HTTP_REQUEST_READ_CMD);
		if (ret != 0)
		{
			goto exit;
		}
		
		ESP_LOGI(TAG, "Receiving HTTP response.\n");
		content_buf[0] = '\0';
		int flag_rsp_ok = tls_receive_http_response(&mbedtls_handler, recv_buf, content_buf, RX_BUFFER_SIZE);

		mbedtls_ssl_close_notify(&mbedtls_handler.ssl);

		if (flag_rsp_ok == 1)
		{
			printf("HTTP response status OK.\n");
			// printf("Response Content: %s\n", content_buf);

			pch = strstr(content_buf, "CMD_");
			if (pch != NULL)
			{
				printf("Received new command: %s\n", content_buf);
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

	exit:
		mbedtls_ssl_session_reset(&mbedtls_handler.ssl);
		mbedtls_net_free(&mbedtls_handler.server_fd);

		if(ret != 0)
		{
			mbedtls_strerror(ret, recv_buf, 100);
			ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, recv_buf);
		}

		putchar('\n'); // JSON output doesn't have a newline at end

		vTaskDelay(COMMAND_RX_CHECK_PERIOD / portTICK_RATE_MS);
	}
}


/* ===== Implementations of private functions ===== */
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id) {
		
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	
	case SYSTEM_EVENT_STA_GOT_IP:
		connect_retry_num = 0;
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	
	case SYSTEM_EVENT_STA_DISCONNECTED:
		if (connect_retry_num < MAX_WIFI_CONNECT_RETRY)
		{
			printf("WiFi disconnected, trying to reconnect %d/%d.\n", connect_retry_num+1, MAX_WIFI_CONNECT_RETRY);
			esp_wifi_connect();
		}
		else
		{
			printf("WiFi tried to reconnect %d times and failed. Not trying anymore.\n", MAX_WIFI_CONNECT_RETRY);
		}
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	
	default:
		break;
	}
   
	return ESP_OK;
}


static int configure_tls(mbedtls_connection_handler_t* mbedtls_handler, const uint8_t* cert_start, const uint8_t* cert_end)
{
	int ret;
	mbedtls_ssl_init(&mbedtls_handler->ssl);
	mbedtls_x509_crt_init(&mbedtls_handler->cacert);
	mbedtls_ctr_drbg_init(&mbedtls_handler->ctr_drbg);

	mbedtls_ssl_config_init(&mbedtls_handler->conf);

	// seed the random number generator
	mbedtls_entropy_init(&mbedtls_handler->entropy);
	ret = mbedtls_ctr_drbg_seed(&mbedtls_handler->ctr_drbg, mbedtls_entropy_func, 
								&mbedtls_handler->entropy, NULL, 0);
	if(ret != 0)
	{
		printf("mbedtls_ctr_drbg_seed returned %d", ret);
		abort();
	}

	// load the CA root certificate
	ret = mbedtls_x509_crt_parse(&mbedtls_handler->cacert, cert_start, cert_end-cert_start);
	if(ret < 0)
	{
		printf("mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
		abort();
	}

	// set hostname for TLS session (it should match CN in server certificate)
	ret = mbedtls_ssl_set_hostname(&mbedtls_handler->ssl, WEB_SERVER);
	if(ret != 0)
	{
		printf("mbedtls_ssl_set_hostname returned -0x%x", -ret);
		abort();
	}

	// set up SSL/TLS structure
	ret = mbedtls_ssl_config_defaults(&mbedtls_handler->conf, MBEDTLS_SSL_IS_CLIENT, 
		MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if(ret != 0)
	{
		printf("mbedtls_ssl_config_defaults returned %d", ret);
		abort();
	}

	// set read/receive timeout to 1 second (1000 ms)
	mbedtls_ssl_conf_read_timeout(&mbedtls_handler->conf, 1000);

	/* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
	   a warning if CA verification fails but it will continue to connect.
	   You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
	*/
	mbedtls_ssl_conf_authmode(&mbedtls_handler->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&mbedtls_handler->conf, &mbedtls_handler->cacert, NULL);
	mbedtls_ssl_conf_rng(&mbedtls_handler->conf, mbedtls_ctr_drbg_random, &mbedtls_handler->ctr_drbg);

	ret = mbedtls_ssl_setup(&mbedtls_handler->ssl, &mbedtls_handler->conf);
	if (ret != 0)
	{
		printf("mbedtls_ssl_setup returned -0x%x\n\n", -ret);
		abort();
	}

	return ret;
}


static int tls_send_http_request(mbedtls_connection_handler_t* mbedtls_handler, const char* server, const char* port, char* http_request)
{
	int ret_value, flags;
	char cert_info[100];

	mbedtls_net_init(&mbedtls_handler->server_fd);

	printf("Connecting to %s:%s.\n", server, port);
	ret_value = mbedtls_net_connect(&mbedtls_handler->server_fd, server,
									port, MBEDTLS_NET_PROTO_TCP);
	if (ret_value != 0)
	{
		printf("mbedtls_net_connect returned -%x", -ret_value);
		return ret_value;
	}
	printf("Connected.\n");

	mbedtls_ssl_set_bio(&mbedtls_handler->ssl, &mbedtls_handler->server_fd, 
						mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

	printf("Performing the SSL/TLS handshake.\n");
	while ((ret_value = mbedtls_ssl_handshake(&mbedtls_handler->ssl)) != 0)
	{
		if (ret_value != MBEDTLS_ERR_SSL_WANT_READ && ret_value != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			printf("mbedtls_ssl_handshake returned -0x%x", -ret_value);
			return ret_value;
		}
	}

	printf("Verifying peer X.509 certificate.\n");
	flags = mbedtls_ssl_get_verify_result(&mbedtls_handler->ssl);
	// we should close the connection if it does not return 0
	if (flags != 0)
	{
		printf("Failed to verify peer certificate.\n");
		bzero(cert_info, sizeof(cert_info));
		mbedtls_x509_crt_verify_info(cert_info, sizeof(cert_info), "  ! ", flags);
		printf("Verification info: %s.\n", cert_info);
	}
	else
	{
		printf("Certificate verified.\n");
	}
	printf("Cipher suite is %s.\n", mbedtls_ssl_get_ciphersuite(&mbedtls_handler->ssl));

	printf("Writing HTTP request.\n");
	size_t written_bytes = 0;
	do {
		ret_value = mbedtls_ssl_write(&mbedtls_handler->ssl,
								(const unsigned char *)http_request + written_bytes,
								strlen(http_request) - written_bytes);
		if (ret_value >= 0) 
		{
			// printf("%d bytes written\n", ret_value);
			written_bytes += ret_value;
		} 
		else if (ret_value != MBEDTLS_ERR_SSL_WANT_WRITE && ret_value != MBEDTLS_ERR_SSL_WANT_READ) 
		{
			printf("mbedtls_ssl_write returned -0x%x", -ret_value);
			return ret_value;
		}
	} while(written_bytes < strlen(http_request));

	return 0;
}

static int tls_receive_http_response(mbedtls_connection_handler_t* mbedtls_handler, char* recv_buf, char* content_buf, int buf_size)
{
	int ret = 0;
	int flag_rsp_ok = 0;
	int flag_content = 0;
	char* pch;
	do
	{
		bzero(recv_buf, buf_size);
		ret = mbedtls_ssl_read(&mbedtls_handler->ssl, (unsigned char *)recv_buf, buf_size - 1);

		if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
			return ret;

		if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) 
		{
			ret = 0;
			break;
		}

		if(ret < 0)
		{
			if (ret == -0x6800)
			{
				printf("This is an expected timeout (not an error).\n");
				ret = 0;
			}
			else
			{
				printf("mbedtls_ssl_read returned -0x%x", -ret);
			}

			break;
		}

		if(ret == 0)
		{
			printf("connection closed");
			break;
		}

		if (strstr (recv_buf,"Status") != NULL && strstr (recv_buf,"200 OK") != NULL)
		{
			flag_rsp_ok = 1;
		}

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
	} while(1);

	return flag_rsp_ok;
}
