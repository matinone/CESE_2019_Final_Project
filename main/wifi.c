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
#include "esp_event_loop.h"
#include "esp_log.h"


/* ===== Macros of private constants ===== */
#define ASSOC_LEAVE 8
#define CONFIG_AP_SSID				"ESP32_AP"
#define CONFIG_AP_PASSWORD			"esp32_ap"
#define CONFIG_AP_MAX_CONNECTIONS	4

/* ===== Declaration of private or external variables ===== */
EventGroupHandle_t wifi_event_group;	// event group to synchronize the WIFI TASK with the WIFI DRIVER events

static int connect_retry_num = 0;
static const int CONNECTED_BIT 			= BIT0;
static const int STA_CONNECTED_BIT 		= BIT2;
static const int STA_DISCONNECTED_BIT	= BIT3;
// static const char *TAG = "WIFI_TASK";


/* ===== Prototypes of private functions ===== */
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);


/* ===== Implementations of public functions ===== */
void initialize_wifi(uint8_t first_time, wifi_mode_t wifi_mode)
{	
	// initialize the tcp stack
	tcpip_adapter_init();

	// configuration specific to Access Point (AP) mode
	if (wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_APSTA)
	{
		// stop DHCP server
		ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
		printf("DHCP server stopped.\n");

		// assign a static IP to the network interface
		tcpip_adapter_ip_info_t info;
	    memset(&info, 0, sizeof(info));
		IP4_ADDR(&info.ip, 192, 168, 1, 1);
	    IP4_ADDR(&info.gw, 192, 168, 1, 1);
	    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
		ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
		printf("TCP adapter configured with IP 192.168.1.1/24\n");

		// start the DHCP server
	    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
		printf("DHCP server started\n");
	}

	// should do this only the first time the function is called
	if (first_time == 1)
	{
		// create the event group to handle wifi events
		wifi_event_group = xEventGroupCreate();
		// initialize the wifi event handler
		ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	}
	
	// initialize the wifi stack in the specified mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode));

	// configure the wifi connection for STAtion (STA) mode
	if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_APSTA)
	{
		wifi_config_t wifi_sta_config = {
			.sta = {
				.ssid = CONFIG_WIFI_SSID,
				.password = CONFIG_WIFI_PASSWORD,
			},
		};
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_config));
	}

	// configure the wifi connection for Access Point (AP) mode
	if (wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_APSTA)
	{
		wifi_config_t wifi_ap_config = {
			.ap = {
				.ssid = CONFIG_AP_SSID,
				.password = CONFIG_AP_PASSWORD,
				.ssid_len = 0,
				.authmode = WIFI_AUTH_WPA_WPA2_PSK,
				.max_connection = CONFIG_AP_MAX_CONNECTIONS,
			},
		};
		if (strlen(CONFIG_AP_PASSWORD) == 0) {
			wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
		}

		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
	}

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

	// STA EVENTS
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
	
	// AP EVENTS
	case SYSTEM_EVENT_AP_START:
		// start web server and mDNS here (or somewhere else using this bit)
		break;

	case SYSTEM_EVENT_AP_STACONNECTED:
		printf("New station connected to AP.\n");
		xEventGroupSetBits(wifi_event_group, STA_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		printf("A station disconnected.\n");
		xEventGroupSetBits(wifi_event_group, STA_DISCONNECTED_BIT);
		break;

	default:
		break;
	}
   
	return ESP_OK;
}
