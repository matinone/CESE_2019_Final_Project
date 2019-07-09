/* ===== [mqtt.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "mqtt.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "mqtt_client.h"

/* ===== Macros of private constants ===== */
#define CONFIG_BROKER_URI "mqtts://mqtt.thingspeak.com:8883"


/* ===== Declaration of private or external variables ===== */
extern EventGroupHandle_t wifi_event_group;
extern QueueHandle_t queue_i2c_to_wifi;

extern const uint8_t thingspeak_mqtts_cert_start[] asm("_binary_thingspeak_mqtts_certificate_pem_start");
extern const uint8_t thingspeak_mqtts_cert_end[]   asm("_binary_thingspeak_mqtts_certificate_pem_end");
static const char *TAG = "MQTTS_TASK";
static char* mqtt_publish_topic = "channels/776064/publish/fields/field1/2WBYREDTIXQ6X9PF";
static char* mqtt_subscribe_topic = "channels/776064/subscribe/fields/field2/E5V8ERAC6B0Y8160";

const int WIFI_CONNECTED_BIT = BIT0;
const int MQTT_CONNECTED_BIT = BIT1;

// queue to pass data from the mqtt event handler to the mqtt rx task
QueueHandle_t queue_mqtt_subs_to_rx_task;



/* ===== Prototypes of private functions ===== */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);


/* ===== Implementations of public functions ===== */
void mqtt_publish_task(void *pvParameter)
{
	// create a queue capable of containing a 5 pointers to struct subscription_data_received_t
	queue_mqtt_subs_to_rx_task = xQueueCreate(5, sizeof(mqtt_sub_data_received_t));

	// wait for wifi connection
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

	int msg_id;
	uint8_t queue_rcv_value;
	char str_number[3];
	BaseType_t xStatus;

	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = CONFIG_BROKER_URI,
		.event_handle = mqtt_event_handler,
		.cert_pem = (const char *)thingspeak_mqtts_cert_start,
		.username = "mbrignone",
		.password = "FP650XEYQ5XX0NY7",
		.disable_clean_session = 0,
	};

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);

	xEventGroupWaitBits(wifi_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);

	while(1)
	{
		// Read data from the queue
		xStatus = xQueueReceive( queue_i2c_to_wifi, &queue_rcv_value,  20 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			ESP_LOGI(TAG, "Received from I2C MASTER TASK: %c\n", queue_rcv_value);
			sprintf(str_number, "%c", queue_rcv_value);
			msg_id = esp_mqtt_client_publish(client, mqtt_publish_topic, str_number, 0, 0, 0);
			if (msg_id != -1)
			{
				ESP_LOGI(TAG, "Sent publish successful.\n");
			}
			else
			{
				printf("Error publishing.\n");
			}
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

void mqtt_rx_task(void *pvParameter)
{
	BaseType_t xStatus;
	mqtt_sub_data_received_t mqtt_data_received;

	xEventGroupWaitBits(wifi_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("MQTT Connected (MQTT RX task).\n");

	while(1)
	{
		// Read data from the queue
		xStatus = xQueueReceive( queue_mqtt_subs_to_rx_task, &(mqtt_data_received),  50 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			printf("MQTT RX received data.\n");
			printf("TOPIC = %.*s\r\n", mqtt_data_received.topic_len, mqtt_data_received.topic);
			printf("DATA = %.*s\r\n", mqtt_data_received.data_len, mqtt_data_received.data);
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}


/* ===== Implementations of private functions ===== */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	int msg_id = 0;
	BaseType_t xStatus;
	mqtt_sub_data_received_t mqtt_data_received;

	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			printf("MQTT_EVENT_CONNECTED\n");
			xEventGroupSetBits(wifi_event_group, MQTT_CONNECTED_BIT);
			msg_id = esp_mqtt_client_subscribe(client, mqtt_subscribe_topic, 0);
            printf("Subscribing to topic %s with msg_id = %d.\n", mqtt_subscribe_topic, msg_id);
			break;

		case MQTT_EVENT_DISCONNECTED:
			printf("MQTT_EVENT_DISCONNECTED\n");
			break;

		case MQTT_EVENT_SUBSCRIBED:
			printf("MQTT_EVENT_SUBSCRIBED with msg_id = %d.\n", event->msg_id);
			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			printf("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d.\n", event->msg_id);
			break;

		case MQTT_EVENT_PUBLISHED:
			printf("MQTT_EVENT_PUBLISHED, msg_id=%d.\n", event->msg_id);
			break;

		case MQTT_EVENT_DATA:
			printf("MQTT_EVENT_DATA\n");
			// printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			// printf("DATA=%.*s\r\n", event->data_len, event->data);

			mqtt_data_received.data_len = event->data_len;
			mqtt_data_received.data = event->data;
			mqtt_data_received.topic_len = event->topic_len;
			mqtt_data_received.topic = event->topic;
			xStatus = xQueueSendToBack(queue_mqtt_subs_to_rx_task, &mqtt_data_received, 0);
			if (xStatus != pdPASS)
			{
				printf("Could not send MQTT data_received to the queue.\n");
			}

			break;

		case MQTT_EVENT_ERROR:
			printf("MQTT_EVENT_ERROR\n");
			break;

		default:
			printf("Other event - id: %d.\n", event->event_id);
			break;
	}
	return ESP_OK;
}
