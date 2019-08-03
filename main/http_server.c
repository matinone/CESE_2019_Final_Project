#include "http_server.h"
#include "http_parser.h"

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

// HTTP headers and web pages
const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const static char http_png_hdr[] = "HTTP/1.1 200 OK\nContent-type: image/png\n\n";
const static char http_wifi_hml[] = "<HTML>"
    "<HEAD>"
        "<TITLE>ESP32 Web Server</TITLE>"
"<meta content=\"width=device-width,initial-scale=1\"name=viewport>"
"<style>div{width:230px;height:300px;position:absolute;top:0;bottom:0;left:0;right:0;margin:auto}</style>"
"<link rel=\"icon\" href=\"data:;base64,=\">"
    "</HEAD>"
"<BODY>"
    "<CENTER>"
"<div>"
"<h1 align=center>ESP32 WiFi Credentials</h1><a href=off.html><img src=on.png></a>"
    "<form action=\"/form_page\" method=\"post\">"
        "SSID: <input type=\"text\" name=\"ssid\"><br>"
        "Password: <input type=\"text\" name=\"password\"><br>"
        "<input type=\"submit\" value=\"Connect to WiFi\">"
    "</form>"
"</div>"
    "</CENTER>" 
"</BODY>"
"</HTML>";

// embedded binary data
extern const uint8_t on_png_start[] asm("_binary_on_png_start");
extern const uint8_t on_png_end[]   asm("_binary_on_png_end");
// extern const uint8_t off_png_start[] asm("_binary_off_png_start");
// extern const uint8_t off_png_end[]   asm("_binary_off_png_end");

static void http_server_netconn_serve(struct netconn *conn);

// HTTP server task
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

static void http_server_netconn_serve(struct netconn *conn)
{
    struct netbuf* inbuf;
    char *buf;
    uint16_t buflen;
    err_t err;

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
                    netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                    printf("Sending default page, relay is OFF\n");
                    netconn_write(conn, http_wifi_hml, sizeof(http_wifi_hml) - 1, NETCONN_NOCOPY);
                }         
                // OFF page
                else if(strcmp(req->resource, "/off.html") == 0) 
                {
                    printf("Sending OFF page...\n");
                    netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                    netconn_write(conn, http_wifi_hml, sizeof(http_wifi_hml) - 1, NETCONN_NOCOPY);
                }
                // ON image
                else if(strcmp(req->resource, "/on.png") == 0) 
                {
                    printf("Sending ON image...\n");
                    netconn_write(conn, http_png_hdr, sizeof(http_png_hdr) - 1, NETCONN_NOCOPY);
                    netconn_write(conn, on_png_start, on_png_end - on_png_start, NETCONN_NOCOPY);
                }
                else
                {
                    printf("Unknown resource: %s\n", req->resource);
                }
            }
            // HTTP POST method
            else if (req->method == POST)
            {
                if(strcmp(req->resource, "/form_page") == 0) 
                {
                    printf("Received form with body: \n");
                    puts(req->body);
                    netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                    netconn_write(conn, http_wifi_hml, sizeof(http_wifi_hml) - 1, NETCONN_NOCOPY);
                }
                else
                {
                    printf("Unknown resource: %s\n", req->resource);
                }
            }
            else
            {
                printf("Unknown method: %d\n", req->method);
            } 
        }
        else 
        {
            printf("Unknown request\n");
        }

        free_request(req);
    }
    
    // close the connection and free the buffer
    netconn_close(conn);
    netbuf_delete(inbuf);
}
