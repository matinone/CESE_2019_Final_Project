#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "nvs_flash.h"

// UART driver
#include "driver/uart.h"


// Event group
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

// HTTP request
char *REQUEST = "GET "CONFIG_RESOURCE" HTTP/1.1\r\n"
	"Host: "CONFIG_WEBSITE"\r\n"
	"User-Agent: ESP32\r\n"
	"\r\n";

// Wifi event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id) {
		
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	
	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	
	default:
		break;
	}
   
	return ESP_OK;
}


// Main task
void main_task(void *pvParameter)
{
	// wait for connection
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("connected!\n");
	printf("\n");
	
	// print the local IP address
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
	printf("\n");
	
	// define connection parameters
	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	
	// address info struct and receive buffer
	struct addrinfo *res;
	char recv_buf[100];
	
	// resolve the IP of the target website
	int result = getaddrinfo(CONFIG_WEBSITE, "80", &hints, &res);
	if((result != 0) || (res == NULL)) {
		printf("Unable to resolve IP for target website %s\n", CONFIG_WEBSITE);
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}
	printf("Target website's IP resolved for target website %s\n", CONFIG_WEBSITE);
	
	char* uart_rcv_buffer = (char*) malloc(1);
	char request_buffer[strlen(REQUEST)];

	while (1)
	{
		// Read data from the UART
		int rcv_len = uart_read_bytes(UART_NUM_0, (uint8_t*)uart_rcv_buffer, 1, 20 / portTICK_RATE_MS);
		if (rcv_len > 0)
		{
			printf("\n\nReceived from UART: %d\n\n", *uart_rcv_buffer);
			sprintf(request_buffer, REQUEST, *uart_rcv_buffer);

			// create a new socket
			int s = socket(res->ai_family, res->ai_socktype, 0);
			if(s < 0) {
				printf("Unable to allocate a new socket\n");
				while(1) vTaskDelay(1000 / portTICK_RATE_MS);
			}
			printf("Socket allocated, id=%d\n", s);

			struct timeval timeout = {
				.tv_sec = 1,
				.tv_usec = 1000000,
				};

			lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			
			// connect to the specified server
			int con_result = connect(s, res->ai_addr, res->ai_addrlen);
			if(con_result != 0) {
				printf("Unable to connect to the target website\n");
				close(s);
				while(1) vTaskDelay(1000 / portTICK_RATE_MS);
			}
			printf("Connected to the target website\n");

			// send the request
			result = write(s, request_buffer, strlen(request_buffer));
			if(result < 0) {
				printf("Unable to send the HTTP request\n");
				close(s);
				while(1) vTaskDelay(1000 / portTICK_RATE_MS);
			}
			printf("HTTP request sent:  %s\n", request_buffer);
			
			// print the response
			printf("HTTP response:\n");
			printf("--------------------------------------------------------------------------------\n");
			int r;
			do {
				bzero(recv_buf, sizeof(recv_buf));
				r = read(s, recv_buf, sizeof(recv_buf) - 1);
				for(int i = 0; i < r; i++) {
					putchar(recv_buf[i]);
				}
			} while(r > 0); 
			printf("--------------------------------------------------------------------------------\n");

			close(s);
		}   // if (rcv_len > 0)
		else 
		{
			printf("Nothing received.\n");
		}

		vTaskDelay(1000 / portTICK_RATE_MS);
	}   // while(1)
}


// Main application
void app_main()
{
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	// create the event group to handle wifi events
	wifi_event_group = xEventGroupCreate();
		
	// initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	
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
	printf("Connecting to %s... ", CONFIG_WIFI_SSID);
	
	// configure the UART0 controller
	uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

	// start the main task
	xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}
