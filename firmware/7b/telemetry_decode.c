#include "telemetry_decode.h"
#include "vesc_can/buffer.h"
#include "vesc_can/vesc_datatypes.h"
#include <math.h>
bool telemetry_decode(const uint8_t *d,unsigned len,telemetry_t *out) {
    if(!d || !out || len!=30 || d[0]!=COMM_GET_VALUES_SETUP_SELECTIVE) return false;
    int32_t at=1;
    if(buffer_get_uint32(d,&at)!=TELEMETRY_MASK) return false;
    telemetry_t s={0};
    s.ecu_temp=buffer_get_float16(d,10,&at);
    s.motor_temp=buffer_get_float16(d,10,&at);
    s.current=buffer_get_float32(d,100,&at);
    s.speed=buffer_get_float32(d,1000,&at)*3.6f;
    s.voltage=buffer_get_float16(d,10,&at);
    s.soc=buffer_get_float16(d,1000,&at)*100;
    s.trip=buffer_get_float32(d,1000,&at)/1000;
    s.fault=d[at++]; s.odo=buffer_get_uint32(d,&at)/1000.0f;
    if(!isfinite(s.speed)||!isfinite(s.current)||s.soc<0||s.soc>100||s.voltage<0) return false;
    *out=s; return true;
}
