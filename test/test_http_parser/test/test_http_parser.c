/* ===== [test_http_parser.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 1.0.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "unity.h"
#include "custom_http_parser.h"

#include <string.h>

/* ===== Macros of private constants ===== */
#define NUMBER_OF_GET_HEADERS   9

/* ===== Declaration of private or external variables ===== */
static char http_get_request[] = "GET /index.html HTTP/1.1\r\n"
"Host: 192.168.1.1\r\n"
"Connection: keep-alive\r\n"
"Upgrade-Insecure-Requests: 1\r\n"
"Save-Data: on\r\n"
"User-Agent: Mozilla/5.0 (Linux; Android 7.0; HUAWEI VNS-L23) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/76.0.3809.111 Mobile Safari/537.36\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3\r\n"
"Referer: http://192.168.1.1/\r\n"
"Accept-Encoding: gzip, deflate\r\n"
"Accept-Language: es-US,es;q=0.9,es-419;q=0.8,en;q=0.7\r\n"
"\r\n";

static char http_post_request[] = "POST /form_page HTTP/1.1\r\n"
"Host: 192.168.1.1\r\n"
"Connection: keep-alive\r\n"
"Content-Length: 37\r\n"
"\r\n"
"name=my_name&id=my_id";

static char http_unsopported_request[] = "PUT /index.html HTTP/1.1\r\n"
"Host: 192.168.1.1\r\n"
"\r\n";

static char http_empty_header_request[] = "GET /index.html HTTP/1.1\r\n"
"\r\n";

static http_request_t* get_request;

/* ===== Implementations of public functions ===== */
void setUp(void)    {
    get_request = parse_http_request(http_get_request, strlen(http_get_request));
}

void tearDown(void) {
    free_request(get_request);
}

void test_request_not_null(void)    {
    TEST_ASSERT_NOT_NULL(get_request);
}

void test_parse_get_method(void)    {
    TEST_ASSERT_EQUAL(GET, get_request->method);
}

void test_parse_post_method(void)    {
    http_request_t* post_request = parse_http_request(http_post_request, strlen(http_post_request));
    TEST_ASSERT_NOT_NULL(post_request);
    TEST_ASSERT_EQUAL(POST, post_request->method);
    free_request(post_request);
}

void test_parse_unsopported_method(void)    {
    http_request_t* unsupported_request = parse_http_request(http_unsopported_request, strlen(http_unsopported_request));
    TEST_ASSERT_NOT_NULL(unsupported_request);
    TEST_ASSERT_EQUAL(UNSUPPORTED, unsupported_request->method);
    free_request(unsupported_request);
}

void test_parse_resource(void)  {
    TEST_ASSERT_EQUAL_STRING("/index.html", get_request->resource);
}

void test_parse_version(void)   {
    TEST_ASSERT_EQUAL_STRING("HTTP/1.1", get_request->version);
}

void test_parse_correct_number_of_headers(void) {
    uint8_t number_of_headers = 0;
    http_header_t* current_header = get_request->headers;
    while(current_header != NULL)   {
        number_of_headers++;
        current_header = current_header->next;
    }

    TEST_ASSERT_EQUAL(NUMBER_OF_GET_HEADERS, number_of_headers);
}

void test_parse_correct_headers(void)   {
    http_header_t* current_header = get_request->headers;

    TEST_ASSERT_EQUAL_STRING("Accept-Language", current_header->name);
    TEST_ASSERT_EQUAL_STRING("es-US,es;q=0.9,es-419;q=0.8,en;q=0.7", current_header->value);
    
    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("Accept-Encoding", current_header->name);
    TEST_ASSERT_EQUAL_STRING("gzip, deflate", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("Referer", current_header->name);
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.1/", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("Accept", current_header->name);
    TEST_ASSERT_EQUAL_STRING("text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("User-Agent", current_header->name);
    TEST_ASSERT_EQUAL_STRING("Mozilla/5.0 (Linux; Android 7.0; HUAWEI VNS-L23) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/76.0.3809.111 Mobile Safari/537.36", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("Save-Data", current_header->name);
    TEST_ASSERT_EQUAL_STRING("on", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("Upgrade-Insecure-Requests", current_header->name);
    TEST_ASSERT_EQUAL_STRING("1", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("Connection", current_header->name);
    TEST_ASSERT_EQUAL_STRING("keep-alive", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_EQUAL_STRING("Host", current_header->name);
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", current_header->value);

    current_header = current_header->next;
    TEST_ASSERT_NULL(current_header);
}

void test_empty_headers(void)   {
    http_request_t* empty_header_request = parse_http_request(http_empty_header_request, strlen(http_empty_header_request));
    TEST_ASSERT_NULL(empty_header_request->headers);
    free_request(empty_header_request);
}

void test_empty_body_get_request(void)  {
    TEST_ASSERT_EQUAL_STRING("", get_request->body);
}

void test_parse_body_post_method(void)    {
    http_request_t* post_request = parse_http_request(http_post_request, strlen(http_post_request));
    TEST_ASSERT_NOT_NULL(post_request);
    TEST_ASSERT_EQUAL_STRING("name=my_name&id=my_id", post_request->body);
    free_request(post_request);
}
