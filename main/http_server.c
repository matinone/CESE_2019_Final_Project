#include "http_server.h"

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
// const static char http_off_hml[] = "<meta content=\"width=device-width,initial-scale=1\"name=viewport><style>div{width:230px;height:300px;position:absolute;top:0;bottom:0;left:0;right:0;margin:auto}</style><link rel=\"icon\" href=\"data:;base64,=\"><div><h1 align=center>Relay is OFF</h1><a href=on.html><img src=on.png></a></div>";
const static char http_off_hml[] = "<meta content=\"width=device-width,initial-scale=1\"name=viewport>"
"<style>div{width:230px;height:300px;position:absolute;top:0;bottom:0;left:0;right:0;margin:auto}</style>"
"<link rel=\"icon\" href=\"data:;base64,=\">"
"<div><h1 align=center>Pagina principal</h1><a href=on.html><img src=on.png></a><a href=hola.html>Texto de prueba</a></div>";

const static char http_on_hml[] = "<meta content=\"width=device-width,initial-scale=1\"name=viewport><style>div{width:230px;height:300px;position:absolute;top:0;bottom:0;left:0;right:0;margin:auto}</style><link rel=\"icon\" href=\"data:;base64,=\"><div><h1 align=center>Relay is ON</h1><a href=off.html><img src=off.png></a></div>"; 

// embedded binary data
extern const uint8_t on_png_start[] asm("_binary_on_png_start");
extern const uint8_t on_png_end[]   asm("_binary_on_png_end");
extern const uint8_t off_png_start[] asm("_binary_off_png_start");
extern const uint8_t off_png_end[]   asm("_binary_off_png_end");

// actual relay status
bool relay_status;

static void http_server_netconn_serve(struct netconn *conn);

// HTTP server task
void http_server(void *pvParameters) 
{
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	printf("* HTTP Server listening\n");
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
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
	  
		netbuf_data(inbuf, (void**)&buf, &buflen);
		
		// extract the first line, with the request
		char *first_line = strtok(buf, "\n");
		
		if(first_line) 
		{
			// default page
			if(strstr(first_line, "GET / ")) {
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				if(relay_status) {
					printf("Sending default page, relay is ON\n");
					netconn_write(conn, http_on_hml, sizeof(http_on_hml) - 1, NETCONN_NOCOPY);
				}					
				else {
					printf("Sending default page, relay is OFF\n");
					netconn_write(conn, http_off_hml, sizeof(http_off_hml) - 1, NETCONN_NOCOPY);
				}
			}
			
			// ON page
			else if(strstr(first_line, "GET /on.html ")) {
				printf("Sending ON page...\n");
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, http_on_hml, sizeof(http_on_hml) - 1, NETCONN_NOCOPY);
			}			

			// OFF page
			else if(strstr(first_line, "GET /off.html ")) {
				printf("Sending OFF page...\n");
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, http_off_hml, sizeof(http_off_hml) - 1, NETCONN_NOCOPY);
			}
			
			// ON image
			else if(strstr(first_line, "GET /on.png ")) {
				printf("Sending ON image...\n");
				netconn_write(conn, http_png_hdr, sizeof(http_png_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, on_png_start, on_png_end - on_png_start, NETCONN_NOCOPY);
			}
			
			// OFF image
			else if(strstr(first_line, "GET /off.png ")) {
				printf("Sending OFF image...\n");
				netconn_write(conn, http_png_hdr, sizeof(http_png_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, off_png_start, off_png_end - off_png_start, NETCONN_NOCOPY);
			}

			// hola ref
			else if(strstr(first_line, "GET /hola.html ")) {
				printf("Sending nothing, someone just clicked the link...\n");
				// netconn_write(conn, http_png_hdr, sizeof(http_png_hdr) - 1, NETCONN_NOCOPY);
				// netconn_write(conn, off_png_start, off_png_end - off_png_start, NETCONN_NOCOPY);
			}
			
			else printf("Unkown request: %s\n", first_line);
		}
		else printf("Unkown request\n");
	}
	
	// close the connection and free the buffer
	netconn_close(conn);
	netbuf_delete(inbuf);
}
