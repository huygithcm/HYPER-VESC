#pragma once
#include "backend.h"
#define TELEMETRY_MASK ((1u<<0)|(1u<<1)|(1u<<2)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<14)|(1u<<16)|(1u<<20))
#ifdef __cplusplus
extern "C" {
#endif
bool telemetry_decode(const uint8_t *data, unsigned length, telemetry_t *out);
#ifdef __cplusplus
}
#endif
