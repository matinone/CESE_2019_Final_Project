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
#include "slave_sim_task.h"
#include "jwt_token.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "lwip/apps/sntp.h"

/* ===== Macros of private constants ===== */
#ifdef CONFIG_THINGSPEAK
	#define CONFIG_BROKER_URI 				"mqtts://mqtt.thingspeak.com:8883"
	#define MQTT_PUBLISH_TOPIC 				"channels/776064/publish/fields/field1/2WBYREDTIXQ6X9PF"
	#define MQTT_SUBSCRIBE_TOPIC 			"channels/776064/subscribe/fields/field2/E5V8ERAC6B0Y8160"
	#define MQTT_USERNAME 					"mbrignone"
	#define MQTT_PASSWORD 					"FP650XEYQ5XX0NY7"
	#define BINARY_CERTIFICATE_START 		"_binary_thingspeak_mqtts_certificate_pem_start"
	#define BINARY_CERTIFICATE_END 			"_binary_thingspeak_mqtts_certificate_pem_end"
	// ThingSpeak free cloud requires some time between transactions (15 seconds)
	#define MQTT_TRANSACTION_WAIT_TIME		15000
#else
	#define MQTT_TRANSACTION_WAIT_TIME		1000
	#ifdef CONFIG_ADAFRUIT
		#define CONFIG_BROKER_URI 			"mqtts://io.adafruit.com:8883"
		#define MQTT_PUBLISH_TOPIC_TX 		"mbrignone/feeds/command-received"
		#define MQTT_PUBLISH_TOPIC_STATUS 	"mbrignone/feeds/status"
		#define MQTT_SUBSCRIBE_TOPIC_RX 	"mbrignone/feeds/command-sent"
		#define MQTT_USERNAME 				"mbrignone"
		#define MQTT_PASSWORD 				"01d6fd13e8af4ed3b30f580e945f5561"
		#define BINARY_CERTIFICATE_START 	"_binary_adafruit_mqtts_certificate_pem_start"
		#define BINARY_CERTIFICATE_END 		"_binary_adafruit_mqtts_certificate_pem_end"
	#endif
#endif

#define DEVICE_BSAS_KEY_START	"_binary_device_bsas_key_pem_start"
#define DEVICE_BSAS_KEY_END   	"_binary_device_bsas_key_pem_end"
#define GCLOUD_KEY_START    	"_binary_gcloud_cert_pem_start"
#define GCLOUD_MQTT_URI			"mqtts://mqtt.2030.ltsapis.goog:8883"
#define GCLOUD_CLIENT_ID		"projects/gcloud-training-mati/locations/us-central1/registries/iotlab-registry/devices/temp-sensor-buenos-aires"
#define GCLOUD_DEVICE_TOPIC		"/devices/temp-sensor-buenos-aires/events"
#define GCLOUD_PUBLISH_INTERVAL	30000


/* ===== Private structs and enums ===== */
typedef enum {
	CLIENT_TYPE_ADAFRUIT,
	CLIENT_TYPE_GCLOUD,
}	client_type_t;


/* ===== Declaration of private or external variables ===== */
extern EventGroupHandle_t wifi_event_group;
extern QueueHandle_t queue_command_processor_rx;

// Adafruit/Thingspeak variables
extern const uint8_t mqtts_cert_start[] asm(BINARY_CERTIFICATE_START);
extern const uint8_t mqtts_cert_end[]   asm(BINARY_CERTIFICATE_END);
static char* mqtt_publish_topic 		= MQTT_PUBLISH_TOPIC_TX;
static char* mqtt_publish_topic_status 	= MQTT_PUBLISH_TOPIC_STATUS;
static char* mqtt_subscribe_topic 		= MQTT_SUBSCRIBE_TOPIC_RX;

// Google Cloud variables
extern const uint8_t device_bsas_key_start[] 	asm(DEVICE_BSAS_KEY_START);
extern const uint8_t device_bsas_key_end[]   	asm(DEVICE_BSAS_KEY_END);
extern const uint8_t gcloud_cert_start[]   		asm(GCLOUD_KEY_START);

// event bits
const int WIFI_CONNECTED_BIT 			= BIT0;
const int MQTT_ADAFRUIT_CONNECTED_BIT 	= BIT1;
const int MQTT_GCLOUD_CONNECTED_BIT 	= BIT2;

// queue to pass data from the mqtt event handler to the mqtt rx task
QueueHandle_t queue_mqtt_subs_to_rx_task;
QueueHandle_t queue_mqtt_tx;
QueueHandle_t queue_mqtt_gcloud;

static esp_mqtt_client_handle_t client_adafruit;
static esp_mqtt_client_handle_t client_gcloud;

static const char *TAG_USER_TASK = "MQTTS_USER_TASK";
static const char *TAG_GCLOUD_TASK = "MQTTS_GCLOUD_TASK";

/* ===== Prototypes of private functions ===== */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static void obtain_time(void);


/* ===== Implementations of public functions ===== */
void mqtt_publish_task(void *pvParameter)
{
	// create a queue capable of containing 5 uint8_t values
    queue_mqtt_tx = xQueueCreate(5, sizeof(uint8_t));
    if (queue_mqtt_tx == NULL)	{
        printf("Could not create queue_mqtt_tx.\n");
    }

	// create a queue capable of containing 5 uint8_t values
    queue_mqtt_gcloud = xQueueCreate(5, sizeof(uint8_t));
    if (queue_mqtt_gcloud == NULL)	{
        printf("Could not create queue_mqtt_gcloud.\n");
    }

	// create a queue capable of containing a 5 pointers to struct subscription_data_received_t
	queue_mqtt_subs_to_rx_task = xQueueCreate(5, sizeof(mqtt_sub_data_received_t));

	// wait for wifi connection
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

	int msg_id;
	uint8_t queue_rcv_value;
	// char str_number[3];
	char* command_string_value;
	BaseType_t xStatus;

	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = CONFIG_BROKER_URI,
		.client_id = "dummy",
		.event_handle = mqtt_event_handler,
		.cert_pem = (const char *)mqtts_cert_start,
		.username = MQTT_USERNAME,
		.password = MQTT_PASSWORD,
		.disable_clean_session = 0,
	};

	client_adafruit = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client_adafruit);

	while(1)	{
		// always wait for this
		xEventGroupWaitBits(wifi_event_group, MQTT_ADAFRUIT_CONNECTED_BIT, false, true, portMAX_DELAY);

		// read data from the queue
		xStatus = xQueueReceive(queue_mqtt_tx, &queue_rcv_value,  20 / portTICK_RATE_MS);
		if (xStatus == pdPASS)	{
			// publish to the slave topic if the state was requested
			if(queue_rcv_value == SLAVE_STATE_FRAME)
			{
				// receive the next value, which will be the slave state
				xStatus = xQueueReceive(queue_mqtt_tx, &queue_rcv_value,  50 / portTICK_RATE_MS);
				command_string_value = translate_slave_machine_state(queue_rcv_value);
				ESP_LOGI(TAG_USER_TASK, "Received from Command Processor TASK: %s (%d)\n", command_string_value, queue_rcv_value);
				vTaskDelay(MQTT_TRANSACTION_WAIT_TIME / portTICK_RATE_MS);

				msg_id = esp_mqtt_client_publish(client_adafruit, mqtt_publish_topic_status, command_string_value, 0, 0, 0);
			}
			else
			{
				command_string_value = translate_command_type(queue_rcv_value);
				ESP_LOGI(TAG_USER_TASK, "Received from Command Processor TASK: %s (%d)\n", command_string_value, queue_rcv_value);
				vTaskDelay(MQTT_TRANSACTION_WAIT_TIME / portTICK_RATE_MS);

				msg_id = esp_mqtt_client_publish(client_adafruit, mqtt_publish_topic, command_string_value, 0, 0, 0);
			}

			if (msg_id != -1)	{
				ESP_LOGI(TAG_USER_TASK, "Sent publish successful.\n");
			}
			else	{
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

	xEventGroupWaitBits(wifi_event_group, MQTT_ADAFRUIT_CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("MQTT Connected (MQTT RX task).\n");

	while(1)	{
		// always wait for this
		xEventGroupWaitBits(wifi_event_group, MQTT_ADAFRUIT_CONNECTED_BIT, false, true, portMAX_DELAY);

		// read data from the queue (passed from the event handler)
		xStatus = xQueueReceive(queue_mqtt_subs_to_rx_task, &(mqtt_data_received),  50 / portTICK_RATE_MS);
		if (xStatus == pdPASS)	{
			printf("MQTT RX received data.\n");
			printf("TOPIC = %.*s\r\n", mqtt_data_received.topic_len, mqtt_data_received.topic);
			printf("DATA = %.*s\r\n", mqtt_data_received.data_len, mqtt_data_received.data);

			mqtt_command.command = str_to_cmd(mqtt_data_received.data);
			xStatus = xQueueSendToBack(queue_command_processor_rx, &mqtt_command, 1000 / portTICK_RATE_MS);
            if (xStatus != pdPASS)	{
                printf("Could not send the data to the queue.\n");
            }
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}


/* ===== Implementations of public functions ===== */
void mqtt_gcloud_publish_task(void *pvParameter)
{
	BaseType_t xStatus;
	rx_command_t mqtt_command;
    mqtt_command.rx_id = MQTT_GCLOUD;

	// wait for wifi connection
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

	ESP_LOGI(TAG_GCLOUD_TASK, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2016 - 1900)) 
    {
        ESP_LOGI(TAG_GCLOUD_TASK, "Time is not set yet. Using WiFi to get time over SNTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    // char strftime_buf[64];
    // time(&now);
    // localtime_r(&now, &timeinfo);
    // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    // ESP_LOGI(TAG_GCLOUD_TASK, "The current date/time is: %s", strftime_buf);

	ESP_LOGI(TAG_GCLOUD_TASK, "Creating JWT Token.\n");
	char* current_token = createGCPJWT("gcloud-training-mati", device_bsas_key_start, device_bsas_key_end - device_bsas_key_start);
	// check current_token != NULL

	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = GCLOUD_MQTT_URI,
		.client_id = GCLOUD_CLIENT_ID,
		.event_handle = mqtt_event_handler,
		.cert_pem = (const char *)gcloud_cert_start,
		.username = "unused",
		.password = current_token,
		.disable_clean_session = 0,
	};

	client_gcloud = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client_gcloud);

	int msg_id;
	while(1)	
	{
		// always wait for this
		xEventGroupWaitBits(wifi_event_group, MQTT_GCLOUD_CONNECTED_BIT, false, true, portMAX_DELAY);

		// ask the command processor to get the slave status
		mqtt_command.command = CMD_SLAVE_STATUS;
		xStatus = xQueueSendToBack(queue_command_processor_rx, &mqtt_command, 1000 / portTICK_RATE_MS);
		if (xStatus != pdPASS)	{
			ESP_LOGE(TAG_GCLOUD_TASK, "Could not send the data to the queue.\n");
		}

		ESP_LOGI(TAG_GCLOUD_TASK, "Publishing to Google Cloud.\n");

		msg_id = esp_mqtt_client_publish(client_gcloud, GCLOUD_DEVICE_TOPIC, "25", 0, 0, 0);
		if (msg_id != -1)	
		{
			ESP_LOGI(TAG_GCLOUD_TASK, "Sent publish successful.\n");
		}
		else	
		{
			ESP_LOGE(TAG_GCLOUD_TASK, "Error publishing.\n");
		}

		vTaskDelay(GCLOUD_PUBLISH_INTERVAL / portTICK_RATE_MS);
	}
}


void start_custom_mqtt_client()
{
	// wait for wifi connection (max 10 seconds)
	// xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, 10000 / portTICK_RATE_MS);
	esp_mqtt_client_start(client_adafruit);
	esp_mqtt_client_start(client_gcloud);
}


void stop_custom_mqtt_client()
{
	esp_mqtt_client_stop(client_adafruit);
	esp_mqtt_client_stop(client_gcloud);
}


/* ===== Implementations of private functions ===== */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	int msg_id = 0;
	BaseType_t xStatus;
	mqtt_sub_data_received_t mqtt_data_received;
	client_type_t client_type;

	// directly comapare pointers because it is not possible to access client->config->uri
	if (client == client_adafruit)	{
		client_type = CLIENT_TYPE_ADAFRUIT;
	}
	else if (client_gcloud)	{
		client_type = CLIENT_TYPE_GCLOUD;
	}
	else	{
		printf("MQTT event from unknown client, returning with error.\n\n");
		return ESP_FAIL;
	}

	switch (event->event_id)	{
		case MQTT_EVENT_CONNECTED:
			if (client_type == CLIENT_TYPE_ADAFRUIT)
			{
				printf("MQTT_EVENT_CONNECTED to ADAFRUIT\n");
				xEventGroupSetBits(wifi_event_group, MQTT_ADAFRUIT_CONNECTED_BIT);

				printf("Adafruit client\n.");
				msg_id = esp_mqtt_client_subscribe(client, mqtt_subscribe_topic, 0);
				printf("Subscribing to topic %s with msg_id = %d.\n", mqtt_subscribe_topic, msg_id);
			}
			else
			{
				printf("MQTT_EVENT_CONNECTED to GCLOUD\n");
				xEventGroupSetBits(wifi_event_group, MQTT_GCLOUD_CONNECTED_BIT);
			}
			break;

		case MQTT_EVENT_DISCONNECTED:
			printf("MQTT_EVENT_DISCONNECTED (client %d)\n", client_type);
			break;

		case MQTT_EVENT_SUBSCRIBED:
			printf("MQTT_EVENT_SUBSCRIBED with msg_id = %d (client %d).\n", event->msg_id, client_type);
			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			printf("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d (client %d).\n", event->msg_id, client_type);
			break;

		case MQTT_EVENT_PUBLISHED:
			printf("MQTT_EVENT_PUBLISHED, msg_id=%d (client %d).\n", event->msg_id, client_type);
			break;

		case MQTT_EVENT_DATA:
			if (client_type == CLIENT_TYPE_ADAFRUIT)
			{
				printf("MQTT_EVENT_DATA from ADAFRUIT\n");
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
			}

			break;

		case MQTT_EVENT_ERROR:
			printf("MQTT_EVENT_ERROR (client %d).\n", client_type);
			break;

		default:
			printf("Other event - id: %d (client %d).\n", event->event_id, client_type);
			break;
	}
	return ESP_OK;
}


static void obtain_time(void)
{
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2019 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG_GCLOUD_TASK, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}
