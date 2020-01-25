/* ===== Avoid multiple inclusion ===== */
#ifndef __JWT_TOKEN__
#define __JWT_TOKEN__

#include <stdint.h>
#include <stdio.h>

char* createGCPJWT(const char* projectId, const uint8_t* privateKey, size_t privateKeySize);

#endif //__JWT_TOKEN__
