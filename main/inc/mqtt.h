
typedef struct {
	int 	data_len;
	char* 	data;
	int 	topic_len;
	char* 	topic;
}	mqtt_sub_data_received_t;

void mqtt_publish_task(void *pvParameter);

void mqtt_rx_task(void *pvParameter);
