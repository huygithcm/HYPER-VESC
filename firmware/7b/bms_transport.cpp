// Native S3 NimBLE transport; JK framing/model copied from the S3 source seed.
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include "backend.h"
#include "bms/bms_jk.h"
#include "bms/bms_model.h"

static portMUX_TYPE peers_lock=portMUX_INITIALIZER_UNLOCKED;
static char names[6][48], addresses[6][18];
static uint8_t types[6];
static int peer_count, selected=-1;
static bool scan_requested;
static NimBLEClient *client;
static NimBLERemoteCharacteristic *characteristic;
static jk_ctx_t parser;
static SemaphoreHandle_t parser_lock;
static Preferences prefs;
static char bound[18]={0};
static uint8_t bound_type;
static bool persisted;
static bool info_received;
static void notification(NimBLERemoteCharacteristic *,uint8_t *data,size_t len,bool) {
    xSemaphoreTake(parser_lock,portMAX_DELAY);
    while(len) {
        bms_snapshot_t s={}; size_t consumed=0;
        jk_feed_result_t r=jk_feed(&parser,data,len,&s,&consumed);
        if(r==JK_FEED_SNAPSHOT) {
            bms_model_publish(&s); bms_model_set_link_state(BMS_LINK_LIVE);
            bms_model_diag_bump(BMS_DIAG_FRAME_OK);
        } else if(r==JK_FEED_DEVICE_INFO) {
            info_received=true;
        } else if(r==JK_FEED_CRC_ERROR) bms_model_diag_bump(BMS_DIAG_CRC_ERROR);
        if(!consumed) break;
        data+=consumed; len-=consumed;
    }
    xSemaphoreGive(parser_lock);
}
static void command(uint8_t cmd) {
    uint8_t data[JK_CMD_LEN];
    jk_build_cmd(data,sizeof(data),cmd,0);
    if(characteristic && client->isConnected()) characteristic->writeValue(data,sizeof(data),false);
}
static void worker(void *) {
    prefs.begin("bms7b",false);
    String saved=prefs.isKey("address")?prefs.getString("address",""):String();
    saved.toCharArray(bound,sizeof(bound));
    bound_type=prefs.getUChar("type",0); persisted=bound[0];
    uint32_t attempt=0, probe=0, kick=0; bool started=false;
    for(;;) {
        bool scan; int choice;
        portENTER_CRITICAL(&peers_lock);
        scan=scan_requested; scan_requested=false; choice=selected; selected=-1;
        portEXIT_CRITICAL(&peers_lock);
        if(scan) {
            bms_model_set_link_state(BMS_LINK_SCANNING);
            auto scanner=NimBLEDevice::getScan(); scanner->setActiveScan(true);
            auto results=scanner->getResults(5000);
            char ns[6][48]={}, as[6][18]={}; uint8_t ts[6]={}; int count=0;
            for(int i=0;i<results.getCount() && count<6;++i) {
                auto d=results.getDevice(i); auto n=d->getName();
                if(n.find("JK")==std::string::npos && !d->isAdvertisingService(NimBLEUUID((uint16_t)0xffe0))) continue;
                snprintf(ns[count],48,"%.24s %d dBm",n.empty()?"JK BMS":n.c_str(),d->getRSSI());
                snprintf(as[count],18,"%s",d->getAddress().toString().c_str());
                ts[count]=d->getAddress().getType(); ++count;
            }
            portENTER_CRITICAL(&peers_lock);
            memcpy(names,ns,sizeof(names)); memcpy(addresses,as,sizeof(addresses)); memcpy(types,ts,sizeof(types)); peer_count=count;
            portEXIT_CRITICAL(&peers_lock);
            scanner->clearResults();
            bms_model_set_link_state(client->isConnected()?BMS_LINK_LIVE:BMS_LINK_UNBOUND);
        }
        if(choice>=0 && choice<peer_count) {
            if(client->isConnected()) client->disconnect();
            portENTER_CRITICAL(&peers_lock);
            strcpy(bound,addresses[choice]); bound_type=types[choice];
            portEXIT_CRITICAL(&peers_lock);
            persisted=false; attempt=0; bms_model_reset();
        }
        if(bound[0] && !client->isConnected() && (!attempt || millis()-attempt>10000)) {
            attempt=millis(); started=false; characteristic=nullptr;
            xSemaphoreTake(parser_lock,portMAX_DELAY); jk_init(&parser); info_received=false; xSemaphoreGive(parser_lock);
            bms_model_set_link_state(BMS_LINK_CONNECTING);
            if(client->connect(NimBLEAddress(std::string(bound),bound_type))) {
                auto service=client->getService(NimBLEUUID((uint16_t)0xffe0));
                characteristic=service?service->getCharacteristic(NimBLEUUID((uint16_t)0xffe1)):nullptr;
                if(!characteristic || !characteristic->subscribe(true,notification)) client->disconnect();
                else { bms_model_set_link_state(BMS_LINK_PROBING); probe=millis(); command(JK_CMD_DEVICE_INFO); }
            }
        }
        if(client->isConnected() && characteristic) {
            xSemaphoreTake(parser_lock,portMAX_DELAY); auto proto=jk_get_proto(&parser); bool answered=info_received; xSemaphoreGive(parser_lock);
            if(proto==JK_PROTO_UNKNOWN) {
                if(answered) bms_model_set_link_state(BMS_LINK_UNSUPPORTED);
                else if(millis()-probe>3000) {probe=millis();command(JK_CMD_DEVICE_INFO);}
            } else {
                if(!persisted) {prefs.putString("address",bound);prefs.putUChar("type",bound_type);persisted=true;}
                if(!started || (bms_model_age_ms()>15000 && millis()-kick>15000)) {
                    command(JK_CMD_CELL_INFO); kick=millis(); started=true;
                }
            }
        } else if(bound[0]) bms_model_set_link_state(BMS_LINK_STALE);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
void bms_begin() {
    parser_lock=xSemaphoreCreateMutex(); if(!parser_lock) abort();
    NimBLEDevice::init("Super VESC 7B");
    client=NimBLEDevice::createClient(); if(!client) abort();
    client->setConnectTimeout(5000);
    bms_model_set_link_state(BMS_LINK_UNBOUND);
    if(xTaskCreate(worker,"bms_worker",6144,nullptr,2,nullptr)!=pdPASS) abort();
}
void backend_bms_scan(){portENTER_CRITICAL(&peers_lock);scan_requested=true;portEXIT_CRITICAL(&peers_lock);}
int backend_bms_devices(char out[6][48]) {
    portENTER_CRITICAL(&peers_lock);memcpy(out,names,sizeof(names));int n=peer_count;portEXIT_CRITICAL(&peers_lock);return n;
}
void backend_bms_select(int index){portENTER_CRITICAL(&peers_lock);selected=index;portEXIT_CRITICAL(&peers_lock);}
