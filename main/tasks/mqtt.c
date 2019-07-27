/* ===== [mqtt.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "mqtt.h"
#include "command_processor.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "mqtt_client.h"

/* ===== Macros of private constants ===== */
#ifdef CONFIG_THINGSPEAK
	#define CONFIG_BROKER_URI "mqtts://mqtt.thingspeak.com:8883"
	#define MQTT_PUBLISH_TOPIC "channels/776064/publish/fields/field1/2WBYREDTIXQ6X9PF"
	#define MQTT_SUBSCRIBE_TOPIC "channels/776064/subscribe/fields/field2/E5V8ERAC6B0Y8160"
	#define MQTT_USERNAME "mbrignone"
	#define MQTT_PASSWORD "FP650XEYQ5XX0NY7"
	#define BINARY_CERTIFICATE_START "_binary_thingspeak_mqtts_certificate_pem_start"
	#define BINARY_CERTIFICATE_END "_binary_thingspeak_mqtts_certificate_pem_end"
#else
	#ifdef CONFIG_ADAFRUIT
		#define CONFIG_BROKER_URI "mqtts://io.adafruit.com:8883"
		#define MQTT_PUBLISH_TOPIC "mbrignone/feeds/test-example"
		#define MQTT_SUBSCRIBE_TOPIC "mbrignone/feeds/test-example"
		#define MQTT_USERNAME "mbrignone"
		#define MQTT_PASSWORD "01d6fd13e8af4ed3b30f580e945f5561"
		#define BINARY_CERTIFICATE_START "_binary_adafruit_mqtts_certificate_pem_start"
		#define BINARY_CERTIFICATE_END "_binary_adafruit_mqtts_certificate_pem_end"
	#endif
#endif


/* ===== Declaration of private or external variables ===== */
extern EventGroupHandle_t wifi_event_group;
extern QueueHandle_t queue_command_processor_rx;

extern const uint8_t thingspeak_mqtts_cert_start[] asm(BINARY_CERTIFICATE_START);
extern const uint8_t thingspeak_mqtts_cert_end[]   asm(BINARY_CERTIFICATE_END);
static const char *TAG = "MQTTS_TASK";
static char* mqtt_publish_topic = MQTT_PUBLISH_TOPIC;
static char* mqtt_subscribe_topic = MQTT_SUBSCRIBE_TOPIC;

const int WIFI_CONNECTED_BIT = BIT0;
const int MQTT_CONNECTED_BIT = BIT1;

// queue to pass data from the mqtt event handler to the mqtt rx task
QueueHandle_t queue_mqtt_subs_to_rx_task;
QueueHandle_t queue_mqtt_tx;

static esp_mqtt_client_handle_t client;

/* ===== Prototypes of private functions ===== */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);


/* ===== Implementations of public functions ===== */
void mqtt_publish_task(void *pvParameter)
{
	// create a queue capable of containing 5 uint8_t values
    queue_mqtt_tx = xQueueCreate(5, sizeof(uint8_t));
    if (queue_mqtt_tx == NULL)
    {
        printf("Could not create queue_mqtt_tx.\n");
    }

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
		.username = MQTT_USERNAME,
		.password = MQTT_PASSWORD,
		.disable_clean_session = 0,
	};

	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);

	// xEventGroupWaitBits(wifi_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);

	while(1)
	{
		// always wait for this
		xEventGroupWaitBits(wifi_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);

		// read data from the queue
		xStatus = xQueueReceive(queue_mqtt_tx, &queue_rcv_value,  20 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			ESP_LOGI(TAG, "Received from Command Processor TASK: %d\n", queue_rcv_value);
			sprintf(str_number, "%d", queue_rcv_value);
			// ThingSpeak free cloud requires some time between transactions (15 seconds)
			vTaskDelay(15000 / portTICK_RATE_MS);
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
	// command to send to the command processor
	rx_command_t mqtt_command;
    mqtt_command.rx_id = MQTT_RX;

	xEventGroupWaitBits(wifi_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("MQTT Connected (MQTT RX task).\n");

	while(1)
	{
		// always wait for this
		xEventGroupWaitBits(wifi_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);

		// read data from the queue (passed from the event handler)
		xStatus = xQueueReceive(queue_mqtt_subs_to_rx_task, &(mqtt_data_received),  50 / portTICK_RATE_MS);
		if (xStatus == pdPASS)
		{
			printf("MQTT RX received data.\n");
			printf("TOPIC = %.*s\r\n", mqtt_data_received.topic_len, mqtt_data_received.topic);
			printf("DATA = %.*s\r\n", mqtt_data_received.data_len, mqtt_data_received.data);

			mqtt_command.command = atoi(mqtt_data_received.data);
			xStatus = xQueueSendToBack(queue_command_processor_rx, &mqtt_command, 1000 / portTICK_RATE_MS);
            if (xStatus != pdPASS)
            {
                printf("Could not send the data to the queue.\n");
            }
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}


void start_custom_mqtt_client()
{
	// wait for wifi connection (max 10 seconds)
	// xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, 10000 / portTICK_RATE_MS);
	esp_mqtt_client_start(client);
}


void stop_custom_mqtt_client()
{
	esp_mqtt_client_stop(client);
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
