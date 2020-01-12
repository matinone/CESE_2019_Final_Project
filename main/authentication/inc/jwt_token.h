/* ===== Avoid multiple inclusion ===== */
#ifndef __JWT_TOKEN__
#define __JWT_TOKEN__

#include <stdint.h>
#include <stdio.h>

#define GCLOUD_CERTIFICATE_START    "_binary_full_gcloud_pem_start"
#define GCLOUD_CERTIFICATE_END      "_binary_full_gcloud_pem_end"

char* createGCPJWT(const char* projectId, const uint8_t* privateKey, size_t privateKeySize);

#endif //__JWT_TOKEN__
