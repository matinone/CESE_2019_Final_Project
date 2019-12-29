/* ===== [command_processor.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "command_processor.h"
#include "wifi.h"
#include "ble_server.h"
#include "mqtt.h"
#include "nvs_storage.h"
#include "serial_protocol_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include <string.h>


/* ===== Macros of private constants ===== */
#define CMD_RX_CHECK_TIME_MS 500

/* ===== Declaration of private or external variables ===== */
QueueHandle_t queue_command_processor_rx;
extern QueueHandle_t queue_uart_tx;
extern QueueHandle_t queue_http_tx;
extern QueueHandle_t queue_tls_https_tx;
extern QueueHandle_t queue_mqtt_tx;
extern QueueHandle_t queue_ble_server_tx;
extern QueueHandle_t queue_i2c_master;

wireless_state_t wireless_state;
static const char* TAG = "COMMAND_PROCESSOR_TASK";

/* ===== Prototypes of private functions ===== */
char* translate_rx_module(rx_module_t module);
char* translate_command_type(command_type_t command);
QueueHandle_t* get_module_queue(rx_module_t module);
rx_module_t wifi_module;


/* ===== Implementations of public functions ===== */
int8_t initialize_command_processor(rx_module_t wifi_type)
{
    wifi_module = wifi_type;
    wireless_state = WIFI_MODE;
    // create a queue capable of containing 5 rx_command_t values
    queue_command_processor_rx = xQueueCreate(5, sizeof(rx_command_t));
    if (queue_command_processor_rx == NULL)
    {
        printf("Could not Command Processor RX QUEUE.\n");
        return -1;
    }
    return 0;
}

void command_processor_task(void *pvParameter)
{   
    rx_command_t current_command;
    BaseType_t xStatus;
    QueueHandle_t* generic_queue_handle_ptr;
    uint8_t master_serial_command;

    while (1)
    {
        // read data from the queue
        xStatus = xQueueReceive(queue_command_processor_rx, &current_command,  portMAX_DELAY);
        if (xStatus == pdPASS) 
        {
            printf("Received command %s from %s.\n", translate_command_type(current_command.command), 
                                                     translate_rx_module(current_command.rx_id));

            // do something here depending on the received command
            switch(current_command.command)
            {
                case CMD_STATUS: ;
                    char* nvs_ssid_value = get_nvs_string_value(WIFI_SSID_NVS_KEY);
                    char* nvs_password_value = get_nvs_string_value(WIFI_PASSWORD_NVS_KEY);

                    printf("SSID = %s, PASSWORD = %s.\n", nvs_ssid_value, nvs_password_value);
                    // release memory allocated in get methods
                    free(nvs_ssid_value);
                    free(nvs_password_value);

                    break;

                case CMD_WIFI:
                    switch(wireless_state)
                    {
                        case WIFI_MODE:
                            printf("Stopping WiFi.\n");
                            stop_wifi();
                            if (wifi_module == MQTT_RX)
                            {
                                stop_custom_mqtt_client();
                            }
                            wireless_state = OFFLINE_MODE;
                            break;
                        case BLE_MODE:
                            printf("Stopping BLE server and starting WiFi.\n");
                            stop_ble_server();
                            initialize_wifi(0, WIFI_MODE_APSTA, get_current_wifi_credentials());
                            if (wifi_module == MQTT_RX)
                            {
                                start_custom_mqtt_client();
                            }
                            wireless_state = WIFI_MODE;
                            break;
                        case OFFLINE_MODE:
                            printf("Starting WiFi.\n");
                            initialize_wifi(0, WIFI_MODE_APSTA, get_current_wifi_credentials());
                            if (wifi_module == MQTT_RX)
                            {
                                start_custom_mqtt_client();
                            }
                            wireless_state = WIFI_MODE;
                            break;
                        default:
                            printf("Invalid wireless state.\n");
                            wireless_state = OFFLINE_MODE;
                    }

                    break;

                case CMD_BLE:
                    switch(wireless_state)
                    {
                        case WIFI_MODE:
                            printf("Stopping WiFi and starting BLE server.\n");
                            stop_wifi();
                            if (wifi_module == MQTT_RX)
                            {
                                stop_custom_mqtt_client();
                            }
                            start_ble_server();
                            wireless_state = BLE_MODE;
                            break;
                        case BLE_MODE:
                            printf("Stopping BLE server.\n");
                            stop_ble_server();
                            wireless_state = OFFLINE_MODE;
                            break;
                        case OFFLINE_MODE:
                            printf("Starting BLE server.\n");
                            start_ble_server();
                            wireless_state = BLE_MODE;
                            break;
                        default:
                            printf("Invalid wireless state.\n");
                            wireless_state = OFFLINE_MODE;
                    }
                    break;
                
                case CMD_ECHO:
                    generic_queue_handle_ptr = get_module_queue(current_command.rx_id);
                    if (generic_queue_handle_ptr != NULL)
                    {
                        xStatus = xQueueSendToBack(*generic_queue_handle_ptr, &current_command.command, 1000 / portTICK_RATE_MS);
                        if (xStatus != pdPASS)
                        {
                            printf("Could not send the data to the queue.\n");
                        }
                    }
                    else
                    {
                        printf("There is no queue handle match for module %s.\n", translate_rx_module(current_command.rx_id));
                    }
                    break;
                case CMD_START:
                    // write to the I2C master queue
                    master_serial_command = COMMAND_START_A;
                    xStatus = xQueueSendToBack(queue_i2c_master, &master_serial_command, 500 / portTICK_RATE_MS);

                    // wait for ack from the master module
                    xStatus = xQueueReceive(queue_command_processor_rx, &current_command,  2000 / portTICK_RATE_MS);
                    if (xStatus == pdPASS && current_command.rx_id == I2C_MASTER_MOD && current_command.command == CMD_OK)
                    {
                        ESP_LOGI(TAG, "Command successfully sent to Slave\n");
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Command could not be sent to Slave\n");
                    }


                    break;
                case CMD_STOP:
                    // write to the I2C master queue

                    break;
                default:
                    break;
            }
        }

        vTaskDelay(CMD_RX_CHECK_TIME_MS / portTICK_RATE_MS);
    }
}


command_type_t str_to_cmd(char* str_command)
{
    if (strstr(str_command, "CMD_START") != NULL)
        return CMD_START;
    if (strstr(str_command, "CMD_STOP") != NULL)
        return CMD_STOP;
    if (strstr(str_command, "CMD_STATUS") != NULL)
        return CMD_STATUS;
    if (strstr(str_command, "CMD_RESTART") != NULL)
        return CMD_RESTART;
    if (strstr(str_command, "CMD_WIFI") != NULL)
        return CMD_WIFI;
    if (strstr(str_command, "CMD_BLE") != NULL)
        return CMD_BLE;
    if (strstr(str_command, "CMD_ECHO") != NULL)
        return CMD_ECHO;

    return CMD_INVALID;
}

/* ===== Implementations of private functions ===== */
char* translate_rx_module(rx_module_t module)
{
    switch(module)
    {
        case HTTP_RX:
            return "HTTP_RX";
        case HTTPS_RX:
            return "HTTPS_RX";
        case MQTT_RX:
            return "MQTT_RX";
        case BLE_SERVER:
            return "BLE_SERVER";
        case UART_RX:
            return "UART_RX";
        case I2C_MASTER_MOD:
            return "I2C_MASTER";
        default:
            return "UNKNOWN";
    }
}

char* translate_command_type(command_type_t command)
{
    switch(command)
    {
        case CMD_START:
            return "CMD_START";
        case CMD_STOP:
            return "CMD_STOP";
        case CMD_STATUS:
            return "CMD_STATUS";
        case CMD_RESTART:
            return "CMD_RESTART";
        case CMD_WIFI:
            return "CMD_WIFI";
        case CMD_BLE:
            return "CMD_BLE";
        case CMD_ECHO:
            return "CMD_ECHO";
        case CMD_INVALID:
            return "CMD_INVALID";
        case CMD_OK:
            return "CMD_OK";
        case CMD_FAIL:
            return "CMD_FAIL";
        default:
            return "UNKNOWN";
    }
}

QueueHandle_t* get_module_queue(rx_module_t module)
{
    switch(module)
    {
        case UART_RX:
            return &queue_uart_tx;
        case HTTP_RX:
            return &queue_http_tx;
        case HTTPS_RX:
            return &queue_tls_https_tx;
        case MQTT_RX:
            return &queue_mqtt_tx;
        case BLE_SERVER:
            return &queue_ble_server_tx;
        default:
            return NULL;
    }
}
