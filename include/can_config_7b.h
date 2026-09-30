#pragma once

#include <stdbool.h>

/* P4 fix/gear-circle-park-reverse, commit 451e139: bus defaults shared by
 * the S3 backend and its settings view. GPIO routing stays in board_7b.h. */
#define CAN_7B_BITRATE_KBPS       500
#define CAN_7B_LOCAL_ID           2
#define CAN_7B_DEFAULT_TARGET_ID  10
#define CAN_7B_POLL_INTERVAL_MS  100
#define CAN_7B_REPLY_TIMEOUT_MS  60
#define CAN_7B_START_DELAY_MS    4000

static inline bool can_7b_target_valid(int id)
{
    /* VESC ID 0 is valid; 255 is broadcast and our own ID cannot be a target. */
    return id >= 0 && id <= 254 && id != CAN_7B_LOCAL_ID;
}
