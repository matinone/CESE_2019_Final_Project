/* ===== Avoid multiple inclusion ===== */
#ifndef __JWT_TOKEN__
#define __JWT_TOKEN__

#include <stdint.h>
#include <stdio.h>

#define DEVICE_BSAS_KEY_START    "_binary_device_bsas_key_pem_start"
#define DEVICE_BSAS_KEY_END      "_binary_device_bsas_key_pem_end"

char* createGCPJWT(const char* projectId, const uint8_t* privateKey, size_t privateKeySize);

#endif //__JWT_TOKEN__
