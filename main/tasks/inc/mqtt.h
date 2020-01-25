/* ===== [mqtt.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __MQTT_H__
#define __MQTT_H__

/* ===== Dependencies ===== */


/* ===== Macros of public constants ===== */


/* ===== Public structs and enums ===== */
/*------------------------------------------------------------------
|  Struct: mqtt_sub_data_received_t
| ------------------------------------------------------------------
|  Description:
|
|  Members:
|		data_len 	-
|		data 		-
|		topic_len 	-
|		topic 		-
*-------------------------------------------------------------------*/
typedef struct {
	int 	data_len;
	char* 	data;
	int 	topic_len;
	char* 	topic;
}	mqtt_sub_data_received_t;


/* ===== Prototypes of public functions ===== */
/*------------------------------------------------------------------
|  Function: mqtt_publish_task
| ------------------------------------------------------------------
|  Description:
|
|  Parameters:
|		-
|
|  Returns:  void
*-------------------------------------------------------------------*/
void mqtt_publish_task(void *pvParameter);


/*------------------------------------------------------------------
|  Function: mqtt_rx_task
| ------------------------------------------------------------------
|  Description:
|
|  Parameters:
|		-
|
|  Returns:  void
*-------------------------------------------------------------------*/
void mqtt_rx_task(void *pvParameter);

void mqtt_gcloud_publish_task(void *pvParameter);


/*------------------------------------------------------------------
|  Function: start_mqtt_custom_client
| ------------------------------------------------------------------
|  Description:
|
|  Parameters:
|		-
|
|  Returns:  void
*-------------------------------------------------------------------*/
void start_custom_mqtt_client();


/*------------------------------------------------------------------
|  Function: stop_mqtt_custom_client
| ------------------------------------------------------------------
|  Description:
|
|  Parameters:
|		-
|
|  Returns:  void
*-------------------------------------------------------------------*/
void stop_custom_mqtt_client();


/* ===== Avoid multiple inclusion ===== */
#endif // __MQTT_H__
