#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "vesc_can/vesc_ride_mode.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    bool fresh;
    float speed, current, voltage, soc, ecu_temp, motor_temp, trip, odo;
    uint8_t fault;
} telemetry_t;
void backend_begin(void);
void backend_loop(void);
void backend_snapshot(telemetry_t *out);
int backend_brightness(void);
int backend_target(void);
void backend_set_brightness(int value);
bool backend_set_target(int value);
void backend_bms_scan(void);
int backend_bms_devices(char names[6][48]);
void backend_bms_select(int index);
void bms_begin(void);
#ifdef __cplusplus
}
#endif
