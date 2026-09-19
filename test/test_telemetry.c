#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "telemetry_decode.h"
#include "vesc_can/buffer.h"
#include "vesc_can/vesc_datatypes.h"
int main(void) {
    uint8_t p[31]={COMM_GET_VALUES_SETUP_SELECTIVE}; int32_t at=1;
    buffer_append_uint32(p,TELEMETRY_MASK,&at);
    buffer_append_float16(p,38.5,10,&at); buffer_append_float16(p,44.5,10,&at);
    buffer_append_float32(p,-2.5,100,&at); buffer_append_float32(p,10,1000,&at);
    buffer_append_int16(p,526,&at); buffer_append_int16(p,820,&at);
    buffer_append_float32(p,24600,1000,&at); p[at++]=7;
    buffer_append_uint32(p,1288000,&at); assert(at==30);
    telemetry_t t={0}; assert(telemetry_decode(p,30,&t));
    assert(fabsf(t.speed-36)<.01 && fabsf(t.current+2.5)<.01);
    assert(fabsf(t.soc-82)<.01 && fabsf(t.trip-24.6)<.01 && t.odo==1288 && t.fault==7);
    for(unsigned i=0;i<30;++i) assert(!telemetry_decode(p,i,&t));
    assert(!telemetry_decode(p,31,&t));
    p[4]^=1; assert(!telemetry_decode(p,30,&t)); p[4]^=1;
    p[0]=0; assert(!telemetry_decode(p,30,&t));
    assert(t.speed==36); // A rejected packet never overwrites the previous snapshot.
    puts("telemetry: valid, units, sign, all truncations, mask and command PASS");
}
