#include "mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#define CONFIG_BROKER_URI "mqtts://mqtt.thingspeak.com:8883"

extern EventGroupHandle_t wifi_event_group;
extern const uint8_t thingspeak_mqtts_cert_start[] asm("_binary_thingspeak_mqtts_certificate_pem_start");
extern const uint8_t thingspeak_mqtts_cert_end[]   asm("_binary_thingspeak_mqtts_certificate_pem_end");

static const char *TAG = "MQTTS_EXAMPLE";

char* mqtt_publish_topic = "channels/776064/publish/fields/field1/2WBYREDTIXQ6X9PF";
char* mqtt_data = "110";

const int WIFI_CONNECTED_BIT = BIT0;
const int MQTT_CONNECTED_BIT = BIT1;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;
	// your_context_t *context = event->context;
	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			xEventGroupSetBits(wifi_event_group, MQTT_CONNECTED_BIT);
			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			break;

		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			printf("DATA=%.*s\r\n", event->data_len, event->data);
			break;

		case MQTT_EVENT_ERROR:
			ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
			break;

		default:
			ESP_LOGI(TAG, "Other event id:%d", event->event_id);
			break;
	}
	return ESP_OK;
}


void mqtt_publish_task(void *pvParameter)
{
	// wait for wifi connection
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

	int msg_id;
	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = CONFIG_BROKER_URI,
		.event_handle = mqtt_event_handler,
		.cert_pem = (const char *)thingspeak_mqtts_cert_start,
	};

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);

	xEventGroupWaitBits(wifi_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);

	printf("Entering while loop.\n");
	while(1)
	{
		msg_id = esp_mqtt_client_publish(client, mqtt_publish_topic, mqtt_data, 0, 0, 0);
		if (msg_id != -1)
		{
			ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		}
		else
		{
			printf("Error publishing.\n");
		}
		vTaskDelay(15000 / portTICK_RATE_MS);
	}
}
