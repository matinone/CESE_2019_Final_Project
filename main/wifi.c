/* ===== [wifi.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "wifi.h"
#include "http_server.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"


/* ===== Macros of private constants ===== */
#define CONFIG_AP_SSID				"ESP32_AP"
#define CONFIG_AP_PASSWORD			"esp32_ap"
#define CONFIG_AP_MAX_CONNECTIONS	1

/* ===== Declaration of private or external variables ===== */
EventGroupHandle_t wifi_event_group;	// event group to synchronize the WIFI TASK with the WIFI DRIVER events

static int connect_retry_num = 0;
static const int CONNECTED_BIT 			= BIT0;
static const int STA_CONNECTED_BIT 		= BIT2;
static const int STA_DISCONNECTED_BIT	= BIT3;

wifi_credential_t current_wifi_credentials = {
		.ssid = CONFIG_WIFI_SSID,
		.password = CONFIG_WIFI_PASSWORD,
};

/* ===== Prototypes of private functions ===== */
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);


/* ===== Implementations of public functions ===== */
void initialize_wifi(uint8_t first_time, wifi_mode_t wifi_mode, wifi_credential_t* wifi_credential)
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
				.ssid = "dummy_ssid",
				.password = "dummy_password",
			},
		};
		memcpy(wifi_sta_config.sta.ssid, wifi_credential->ssid, MAX_WIFI_SSID_SIZE);
		memcpy(wifi_sta_config.sta.password, wifi_credential->password, MAX_WIFI_PASSWORD_SIZE);

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
	printf("Connecting to %s.", wifi_credential->ssid);
}


void stop_wifi()
{
	esp_wifi_disconnect();
	esp_wifi_stop();
	esp_wifi_deinit();
}


wifi_credential_t* get_current_wifi_credentials()
{
	return &current_wifi_credentials;
}

void set_current_wifi_credentials(char* ssid, char* password)
{
	strncpy((char*)&current_wifi_credentials.ssid[0], ssid, MAX_WIFI_SSID_SIZE);
	strncpy((char*)&current_wifi_credentials.password[0], password, MAX_WIFI_PASSWORD_SIZE);
}

/* ===== Implementations of private functions ===== */
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id) {

	// STA EVENTS
	case SYSTEM_EVENT_STA_START:
		connect_retry_num = 0;
		esp_wifi_connect();
		break;
	
	case SYSTEM_EVENT_STA_GOT_IP:
		connect_retry_num = 0;
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	
	case SYSTEM_EVENT_STA_DISCONNECTED:
		// reason == ASSOC_LEAVE means that esp_wifi_disconnect() was called
		if (event->event_info.disconnected.reason == WIFI_REASON_ASSOC_LEAVE)
		{
			printf("WiFi intentionally disconnected, not trying to reconnect.\n");
		}
		else if (event->event_info.disconnected.reason == WIFI_REASON_AUTH_FAIL || 
			event->event_info.disconnected.reason == WIFI_REASON_NO_AP_FOUND)
		{
			printf("Authentication failed while trying to connect (wrong SSID or password).\n");
		}
		else
		{
			if (connect_retry_num < MAX_WIFI_CONNECT_RETRY)
			{
				printf("WiFi disconnected (reason code %d), trying to reconnect %d/%d.\n", 
					event->event_info.disconnected.reason, connect_retry_num+1, MAX_WIFI_CONNECT_RETRY);
				connect_retry_num++;
				esp_wifi_connect();
			}
			else
			{
				printf("WiFi tried to reconnect %d times and failed. Not trying anymore.\n", MAX_WIFI_CONNECT_RETRY);
			}
		}
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	
	// AP EVENTS
	case SYSTEM_EVENT_AP_START:
		// start web server and mDNS here (or somewhere else using this bit)
		// start the HTTP server task
		
		xTaskCreate(&http_server, "http_server", 20000, NULL, 6, NULL);
		printf("HTTP server started\n");

		break;

	case SYSTEM_EVENT_AP_STACONNECTED:
		printf("New station connected to AP.\n");
		if ( (xEventGroupGetBits(wifi_event_group) & CONNECTED_BIT) != 0 )
		{
			printf("Disconnecting station from Access Point.\n");
			esp_wifi_disconnect();
		}
		
		xEventGroupSetBits(wifi_event_group, STA_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		printf("A station disconnected.\n");
		if ( (xEventGroupGetBits(wifi_event_group) & CONNECTED_BIT) == 0 )
		{
			printf("Reconnecting station to Access Point.\n");
			esp_wifi_connect();
		}
		xEventGroupSetBits(wifi_event_group, STA_DISCONNECTED_BIT);
		break;

	default:
		break;
	}
   
	return ESP_OK;
}
