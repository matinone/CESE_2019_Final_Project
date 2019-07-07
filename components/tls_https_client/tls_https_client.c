/* ===== [tls_https_client.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2019
 */

/* ===== Dependencies ===== */
#include "tls_https_client.h"

#include "stdio.h"
#include "string.h"

#include "mbedtls/platform.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/error.h"

/* ===== Macros of private constants ===== */


/* ===== Declaration of private or external variables ===== */


/* ===== Prototypes of private functions ===== */


/* ===== Implementations of public functions ===== */
int configure_tls(mbedtls_connection_handler_t* mbedtls_handler, char* server, const uint8_t* cert_start, const uint8_t* cert_end)
{
	int ret;
	mbedtls_ssl_init(&mbedtls_handler->ssl);
	mbedtls_x509_crt_init(&mbedtls_handler->cacert);
	mbedtls_ctr_drbg_init(&mbedtls_handler->ctr_drbg);

	mbedtls_ssl_config_init(&mbedtls_handler->conf);

	// seed the random number generator
	mbedtls_entropy_init(&mbedtls_handler->entropy);
	ret = mbedtls_ctr_drbg_seed(&mbedtls_handler->ctr_drbg, mbedtls_entropy_func, 
								&mbedtls_handler->entropy, NULL, 0);
	if(ret != 0)
	{
		printf("mbedtls_ctr_drbg_seed returned %d", ret);
		abort();
	}

	// load the CA root certificate
	ret = mbedtls_x509_crt_parse(&mbedtls_handler->cacert, cert_start, cert_end-cert_start);
	if(ret < 0)
	{
		printf("mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
		abort();
	}

	// set hostname for TLS session (it should match CN in server certificate)
	ret = mbedtls_ssl_set_hostname(&mbedtls_handler->ssl, server);
	if(ret != 0)
	{
		printf("mbedtls_ssl_set_hostname returned -0x%x", -ret);
		abort();
	}

	// set up SSL/TLS structure
	ret = mbedtls_ssl_config_defaults(&mbedtls_handler->conf, MBEDTLS_SSL_IS_CLIENT, 
		MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if(ret != 0)
	{
		printf("mbedtls_ssl_config_defaults returned %d", ret);
		abort();
	}

	// set read/receive timeout to 1 second (1000 ms)
	mbedtls_ssl_conf_read_timeout(&mbedtls_handler->conf, 1000);

	/* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
	   a warning if CA verification fails but it will continue to connect.
	   You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
	*/
	mbedtls_ssl_conf_authmode(&mbedtls_handler->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&mbedtls_handler->conf, &mbedtls_handler->cacert, NULL);
	mbedtls_ssl_conf_rng(&mbedtls_handler->conf, mbedtls_ctr_drbg_random, &mbedtls_handler->ctr_drbg);

	ret = mbedtls_ssl_setup(&mbedtls_handler->ssl, &mbedtls_handler->conf);
	if (ret != 0)
	{
		printf("mbedtls_ssl_setup returned -0x%x\n\n", -ret);
		abort();
	}

	return ret;
}

int tls_send_http_request(mbedtls_connection_handler_t* mbedtls_handler, const char* server, const char* port, char* http_request)
{
	int ret_value, flags;
	char cert_info[100];

	mbedtls_net_init(&mbedtls_handler->server_fd);

	printf("Connecting to %s:%s.\n", server, port);
	ret_value = mbedtls_net_connect(&mbedtls_handler->server_fd, server,
									port, MBEDTLS_NET_PROTO_TCP);
	if (ret_value != 0)
	{
		printf("mbedtls_net_connect returned -%x", -ret_value);
		return ret_value;
	}
	printf("Connected.\n");

	mbedtls_ssl_set_bio(&mbedtls_handler->ssl, &mbedtls_handler->server_fd, 
						mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

	printf("Performing the SSL/TLS handshake.\n");
	while ((ret_value = mbedtls_ssl_handshake(&mbedtls_handler->ssl)) != 0)
	{
		if (ret_value != MBEDTLS_ERR_SSL_WANT_READ && ret_value != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			printf("mbedtls_ssl_handshake returned -0x%x", -ret_value);
			return ret_value;
		}
	}

	printf("Verifying peer X.509 certificate.\n");
	flags = mbedtls_ssl_get_verify_result(&mbedtls_handler->ssl);
	// we should close the connection if it does not return 0
	if (flags != 0)
	{
		printf("Failed to verify peer certificate.\n");
		bzero(cert_info, sizeof(cert_info));
		mbedtls_x509_crt_verify_info(cert_info, sizeof(cert_info), "  ! ", flags);
		printf("Verification info: %s.\n", cert_info);
	}
	else
	{
		printf("Certificate verified.\n");
	}
	printf("Cipher suite is %s.\n", mbedtls_ssl_get_ciphersuite(&mbedtls_handler->ssl));

	printf("Writing HTTP request.\n");
	size_t written_bytes = 0;
	do {
		ret_value = mbedtls_ssl_write(&mbedtls_handler->ssl,
								(const unsigned char *)http_request + written_bytes,
								strlen(http_request) - written_bytes);
		if (ret_value >= 0) 
		{
			// printf("%d bytes written\n", ret_value);
			written_bytes += ret_value;
		} 
		else if (ret_value != MBEDTLS_ERR_SSL_WANT_WRITE && ret_value != MBEDTLS_ERR_SSL_WANT_READ) 
		{
			printf("mbedtls_ssl_write returned -0x%x", -ret_value);
			return ret_value;
		}
	} while(written_bytes < strlen(http_request));

	return 0;
}

int tls_receive_http_response(mbedtls_connection_handler_t* mbedtls_handler, char* recv_buf, char* content_buf, int buf_size)
{
	int ret = 0;
	int flag_rsp_ok = 0;
	int flag_content = 0;
	char* pch;
	do
	{
		bzero(recv_buf, buf_size);
		ret = mbedtls_ssl_read(&mbedtls_handler->ssl, (unsigned char *)recv_buf, buf_size - 1);

		if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
			return ret;

		if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) 
		{
			ret = 0;
			break;
		}

		if(ret < 0)
		{
			// expected timeout (not an error)
			if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
			{
				// printf("This is an expected timeout (not an error).\n");
				ret = 0;
			}
			else
			{
				printf("mbedtls_ssl_read returned -0x%x", -ret);
			}

			break;
		}

		if(ret == 0)
		{
			printf("connection closed");
			break;
		}

		if (strstr (recv_buf,"Status") != NULL && strstr (recv_buf,"200 OK") != NULL)
		{
			flag_rsp_ok = 1;
		}

		pch = strstr(recv_buf, "\n\r\n");
		if (pch != NULL || flag_content == 1)
		{
			if (pch != NULL)
			{
				strcat(content_buf, pch+3);	// pch + 3 to ignore the LF+CR+LF
			}
			else
			{
				strcat(content_buf, recv_buf);
			}
			flag_content = 1;
		}
	} while(1);

	return flag_rsp_ok;
}


void tls_clean_up(mbedtls_connection_handler_t* mbedtls_handler, int error)
{
	char error_buffer[50];
	if (error == 0)
	{
		mbedtls_ssl_close_notify(&mbedtls_handler->ssl);
	}
	else
	{
		mbedtls_strerror(error, error_buffer, 50);
		printf("Last error was: -0x%x - %s", -error, error_buffer);
	}

	mbedtls_ssl_session_reset(&mbedtls_handler->ssl);
	mbedtls_net_free(&mbedtls_handler->server_fd);

}


/* ===== Implementations of private functions ===== */
