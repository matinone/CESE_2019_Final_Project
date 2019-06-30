/* ===== [thingspeak_http_request.h] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Avoid multiple inclusion ===== */
#ifndef __THINGSPEAK_HTTP_REQUEST__
#define __THINGSPEAK_HTTP_REQUEST__

/* ===== Dependencies ===== */

/* ===== Macros of public constants ===== */

// write specified character in field 1
char* HTTP_REQUEST_WRITE = "GET /update?api_key=2WBYREDTIXQ6X9PF&field1=%d HTTP/1.1\r\n"
	"Host: api.thingspeak.com\r\n"
	"User-Agent: ESP32\r\n"
	"\r\n";

// read in csv format the entries in the field 1 of the last 5 minutes
char* HTTP_REQUEST_READ = "GET /channels/776064/fields/1.csv?api_key=E5V8ERAC6B0Y8160&minutes=5 HTTP/1.1\r\n"
	"Host: api.thingspeak.com\r\n"
	"User-Agent: ESP32\r\n"
	"\r\n";

// read next command to be executed
char* HTTP_REQUEST_READ_CMD = "POST /talkbacks/33594/commands/execute?api_key=VR1E8X612PYZ5BQS HTTP/1.1\r\n"
	"Host: api.thingspeak.com\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"User-Agent: ESP32\r\n"
	"\r\n";


/* ===== Public structs and enums ===== */

/* ===== Prototypes of public functions ===== */

/* ===== Avoid multiple inclusion ===== */
#endif // __THINGSPEAK_HTTP_REQUEST__
