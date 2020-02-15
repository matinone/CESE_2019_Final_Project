/* ===== Avoid multiple inclusion ===== */
#ifndef __JWT_TOKEN__
#define __JWT_TOKEN__

#include <stdint.h>
#include <stdio.h>

#define TOKEN_PERIOD    3600    // token valid for 3600 seconds (1 hour)

typedef struct {
    char*   token;
    time_t  exp_time;
}   jwt_token_t;

jwt_token_t createGCPJWT(const char* projectId, const uint8_t* privateKey, size_t privateKeySize);

#endif //__JWT_TOKEN__
