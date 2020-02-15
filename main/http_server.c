/* ===== [http_server.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */


/* ===== Dependencies ===== */
#include "http_server.h"
#include "custom_http_parser.h"
#include "nvs_storage.h"
#include "wifi.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

/* ===== Macros of private constants ===== */

/* ===== Declaration of private or external variables ===== */
// HTTP headers and web pages
const static char http_html_header[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const static char http_png_header[] = "HTTP/1.1 200 OK\nContent-type: image/png\n\n";

// embedded binary data
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t wifi_icon_png_start[] asm("_binary_wifi_icon_png_start");
extern const uint8_t wifi_icon_png_end[]   asm("_binary_wifi_icon_png_end");

static const char* TAG = "WEB_SERVER_TASK";

/* ===== Prototypes of private functions ===== */
static void http_server_netconn_serve(struct netconn *conn);
static char* recover_encoded_spaces(char* encoded_string);


/* ===== Implementations of public functions ===== */
void http_server(void *pvParameters) 
{
    struct netconn *conn, *newconn;
    err_t err;

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, 80);
    netconn_listen(conn);
    printf("HTTP Server listening\n");
    do {
        err = netconn_accept(conn, &newconn);
        if (err == ERR_OK) 
        {
            http_server_netconn_serve(newconn);
            netconn_delete(newconn);
        }

        vTaskDelay(100);
    } while(err == ERR_OK);
    
    netconn_close(conn);
    netconn_delete(conn);
}


/* ===== Implementations of private functions ===== */
static void http_server_netconn_serve(struct netconn *conn)
{
    struct netbuf* inbuf;
    char *buf;
    uint16_t buflen;
    err_t err;
    esp_err_t nvs_error;

    err = netconn_recv(conn, &inbuf);
    if (err == ERR_OK) {
        netbuf_data(inbuf, (void**)&buf, &buflen);
        
        http_request_t* req = parse_http_request(buf, buflen);
        if(req) 
        {
            // HTTP GET method
            if (req->method == GET)
            {
                // default page
                if(strcmp(req->resource, "/") == 0) 
                {
                    netconn_write(conn, http_html_header, sizeof(http_html_header) - 1, NETCONN_NOCOPY);
                    // printf("Sending default page, relay is OFF\n");
                    netconn_write(conn, index_html_start, index_html_end - index_html_start, NETCONN_NOCOPY);
                }         
                // OFF page
                else if(strcmp(req->resource, "/index.html") == 0) 
                {
                    // printf("Sending OFF page...\n");
                    netconn_write(conn, http_html_header, sizeof(http_html_header) - 1, NETCONN_NOCOPY);
                    netconn_write(conn, index_html_start, index_html_end - index_html_start, NETCONN_NOCOPY);
                }
                // ON image
                else if(strcmp(req->resource, "/wifi_icon.png") == 0) 
                {
                    // printf("Sending WiFi icon image...\n");
                    netconn_write(conn, http_png_header, sizeof(http_png_header) - 1, NETCONN_NOCOPY);
                    netconn_write(conn, wifi_icon_png_start, wifi_icon_png_end - wifi_icon_png_start, NETCONN_NOCOPY);
                }
                else
                {
                    ESP_LOGE(TAG, "Unknown resource: %s", req->resource);
                }
            }
            // HTTP POST method
            else if (req->method == POST)
            {
                if(strcmp(req->resource, "/form_page") == 0) 
                {
                    ESP_LOGI(TAG, "Received form with body: ");
                    recover_encoded_spaces(req->body);
                    puts(req->body);
                    netconn_write(conn, http_html_header, sizeof(http_html_header) - 1, NETCONN_NOCOPY);
                    netconn_write(conn, index_html_start, index_html_end - index_html_start, NETCONN_NOCOPY);

                    char * pch = strtok (req->body,"=&");   // pch here is "ssid"
                    pch = strtok(NULL, "=&");               // pch here is the ssid value
                    nvs_error = set_nvs_string_value(WIFI_SSID_NVS_KEY, pch);
                    pch = strtok(NULL, "=&");               // pch here is "password"
                    pch = strtok(NULL, "=&");               // pch here is the password value
                    nvs_error = set_nvs_string_value(WIFI_PASSWORD_NVS_KEY, pch);

                    if (nvs_error != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Error writting WiFi credentials in flash memory.");
                    }

                    // restart wifi with new credentials;
                    set_current_wifi_credentials(get_nvs_string_value(WIFI_SSID_NVS_KEY),
                                                get_nvs_string_value(WIFI_PASSWORD_NVS_KEY));
                    stop_wifi();
                    initialize_wifi(0, WIFI_MODE_APSTA, get_current_wifi_credentials());
                }
                else
                {
                    ESP_LOGE(TAG, "Unknown resource: %s .", req->resource);
                }
            }
            else
            {
                ESP_LOGE(TAG, "Unknown method: %d .", req->method);
            } 
        }
        else 
        {
            ESP_LOGE(TAG, "Unknown request.");
        }

        free_request(req);
    }
    
    // close the connection and free the buffer
    netconn_close(conn);
    netbuf_delete(inbuf);
}


static char* recover_encoded_spaces(char* encoded_string)
{
    for(int i=0; i<strlen(encoded_string); i++)
    {
        if (*(encoded_string+i) == '+')
        {
            *(encoded_string+i) = ' ';
        }
    }

    return encoded_string;
}
