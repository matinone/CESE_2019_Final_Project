/* ===== [serial_protocol_common.c] =====
 * Copyright Matias Brignone <mnbrignone@gmail.com>
 * All rights reserved.
 *
 * Version: 0.1.0
 * Creation Date: 2020
 */


/* ===== Dependencies ===== */
#include "serial_protocol_common.h"

/* ===== Macros of private constants ===== */

/* ===== Declaration of private or external variables ===== */

/* ===== Prototypes of private functions ===== */

/* ===== Implementations of public functions ===== */
uint8_t check_frame_format(uint8_t* frame)
{
    return (frame[0] == COMMAND_FRAME_START && frame[COMMAND_FRAME_LENGTH-1] == COMMAND_FRAME_END);
}

/* ===== Implementations of private functions ===== */
