/* ===== [wifi.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "wifi.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"


/* ===== Macros of private constants ===== */
#define ASSOC_LEAVE 8

/* ===== Declaration of private or external variables ===== */
EventGroupHandle_t wifi_event_group;	// event group to synchronize the WIFI TASK with the WIFI DRIVER events

static int connect_retry_num = 0;
static const int CONNECTED_BIT = BIT0;
// static const char *TAG = "WIFI_TASK";


/* ===== Prototypes of private functions ===== */
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);


/* ===== Implementations of public functions ===== */
void initialize_wifi(uint8_t first_time)
{	
	if (first_time == 1)
	{
		// create the event group to handle wifi events
		wifi_event_group = xEventGroupCreate();
		// initialize the wifi event handler
		ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	}
	
	// initialize the tcp stack
	tcpip_adapter_init();
	
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


void stop_wifi()
{
	esp_wifi_disconnect();
	esp_wifi_stop();
	esp_wifi_deinit();
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
		// reason == ASSOC_LEAVE means that esp_wifi_disconnect() was called
		if (event->event_info.disconnected.reason != ASSOC_LEAVE)
		{
			if (connect_retry_num < MAX_WIFI_CONNECT_RETRY)
			{
				printf("WiFi disconnected, trying to reconnect %d/%d.\n", connect_retry_num+1, MAX_WIFI_CONNECT_RETRY);
				esp_wifi_connect();
			}
			else
			{
				printf("WiFi tried to reconnect %d times and failed. Not trying anymore.\n", MAX_WIFI_CONNECT_RETRY);
			}
		}
		else
		{
			printf("WiFi intentionally disconnected, not trying to reconnect.\n");
		}
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	
	default:
		break;
	}
   
	return ESP_OK;
}
