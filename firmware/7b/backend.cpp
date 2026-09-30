#include "backend.h"
#include "board_7b.h"
#include "can_config_7b.h"
#include "io_expander.h"
#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include "vesc_can/comm_can.h"
#include "vesc_can/buffer.h"
#include "telemetry_decode.h"

static Preferences prefs;
static int light=80, target=CAN_7B_DEFAULT_TARGET_ID;
static bool dirty;
static uint32_t changed, received;
static telemetry_t telemetry;
static portMUX_TYPE data_lock=portMUX_INITIALIZER_UNLOCKED;
static const uint32_t mask=TELEMETRY_MASK;
static void packet(const uint8_t *d,unsigned len) {
    vesc_ride_mode_process_response(d,len);
    // A fixed selective mask makes length validation atomic: reject incomplete
    // or unexpected packets before any value can become "fresh".
    telemetry_t s={};
    if(!telemetry_decode(d,len,&s)) return;
    portENTER_CRITICAL(&data_lock); telemetry=s; received=millis(); portEXIT_CRITICAL(&data_lock);
}
static void can_task(void *) {
    uint8_t request[5]={COMM_GET_VALUES_SETUP_SELECTIVE}; int32_t at=1;
    buffer_append_uint32(request,mask,&at);
    // Match P4's quiet window while the ESC Lisp and other CAN nodes start.
    vTaskDelay(pdMS_TO_TICKS(CAN_7B_START_DELAY_MS));
    for(;;) {
        comm_can_send_buffer_sync(target,request,sizeof(request),0,CAN_7B_REPLY_TIMEOUT_MS);
        vesc_ride_mode_poll_loop();
        vTaskDelay(pdMS_TO_TICKS(CAN_7B_POLL_INTERVAL_MS));
    }
}
void backend_begin() {
    prefs.begin("display7b",false);
    light=constrain(prefs.getInt("brightness",80),10,97);
    target=prefs.getInt("target",CAN_7B_DEFAULT_TARGET_ID);
    if(!can_7b_target_valid(target)) {
        Serial.printf("Invalid saved CAN target %d; using %d\n",target,CAN_7B_DEFAULT_TARGET_ID);
        target=CAN_7B_DEFAULT_TARGET_ID;
    }
    ioexpBacklight(light);
    vesc_ride_mode_init(target);
    comm_can_set_packet_handler(packet);
    ESP_ERROR_CHECK(comm_can_start(PIN_CAN_TX,PIN_CAN_RX,CAN_7B_LOCAL_ID,CAN_7B_BITRATE_KBPS));
    comm_can_set_fw_info("Super VESC S3 7B",1,0,nullptr,0);
    if(xTaskCreate(can_task,"can_poll",4096,nullptr,3,nullptr)!=pdPASS) abort();
    Serial.printf("CAN TX%d RX%d %dk target=%d local=%d; poll delay=%dms\n",
                  PIN_CAN_TX,PIN_CAN_RX,CAN_7B_BITRATE_KBPS,target,CAN_7B_LOCAL_ID,
                  CAN_7B_START_DELAY_MS);
}
void backend_snapshot(telemetry_t *out) {
    portENTER_CRITICAL(&data_lock); *out=telemetry; out->fresh=received && millis()-received<1000; portEXIT_CRITICAL(&data_lock);
}
int backend_brightness(){return light;}
int backend_target(){return target;}
void backend_set_brightness(int value) {
    light=constrain(value,10,97); ioexpBacklight(light); dirty=true; changed=millis();
}
bool backend_set_target(int value) {
    // Reboot initializes both telemetry and ride mode with the saved target.
    // Keep the active target unchanged until then to avoid racing the poller.
    if(!can_7b_target_valid(value)) return false;
    return prefs.putInt("target",value)==sizeof(int32_t);
}
void backend_loop() {
    if(dirty && millis()-changed>1500) {
        prefs.putInt("brightness",light); dirty=false;
    }
}
