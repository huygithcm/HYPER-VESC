#include <Arduino.h>
#include <Wire.h>
#include "board_7b.h"
#include "io_expander.h"
#include "gt911.h"
#include "display_port.h"
#include "backend.h"
#include "lvgl.h"
extern "C" {
#include "preview_ui.h"
}
void setup() {
    Serial.begin(115200); delay(300);
    Serial.printf("\nSuper VESC 7B hardware build %s %s\n",__DATE__,__TIME__);
    Serial.printf("FLASH=%u PSRAM=%u\n",ESP.getFlashChipSize(),ESP.getPsramSize());
    if(!psramFound()||ESP.getPsramSize()<8*1024*1024) abort();
    Wire.begin(PIN_I2C_SDA,PIN_I2C_SCL,I2C_HZ);
    if(!ioexpBegin()) {Serial.println("IO expander failed"); abort();}
    ioexpWrite(IOEXP_BACKLIGHT,false);
    ioexpWrite(IOEXP_CAN_SEL,true);
    ioexpWrite(IOEXP_LCD_RST,false); delay(20);
    ioexpWrite(IOEXP_LCD_RST,true); delay(120);
    if(!gt911Begin()) {Serial.println("GT911 failed"); abort();}
    Serial.printf("GT911 id=%s addr=0x%02x\n",gt911ProductId(),gt911Address());
    lv_init();
    if(!display_port_begin()) abort();
    backend_begin();
    bms_begin();
    preview_ui_init();
    preview_ui_startup();
    lv_timer_handler();
    ioexpWrite(IOEXP_BACKLIGHT,true);
    Serial.println("READY 1024x600 RGB565 double framebuffer; hardware UI");
}
void loop() {
    lv_timer_handler(); backend_loop();
    static uint32_t last;
    if(millis()-last>10000){last=millis();display_port_stats();}
    delay(5);
}
