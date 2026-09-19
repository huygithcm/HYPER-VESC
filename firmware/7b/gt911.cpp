#include "gt911.h"

#include <Arduino.h>
#include <Wire.h>

#include "board_7b.h"
#include "io_expander.h"

#define REG_PRODUCT_ID 0x8140
#define REG_STATUS     0x814E
#define REG_POINT1     0x814F

static uint8_t addr = GT911_ADDR;
static char    productId[8] = "?";

static bool readReg(uint16_t reg, uint8_t *buf, size_t len) {
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) return false;   // repeated start
    if (Wire.requestFrom((int)addr, (int)len) != (int)len) return false;
    for (size_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

static bool writeReg(uint16_t reg, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool gt911Begin() {
    // GT911 chot dia chi I2C ngay luc nha reset, dua theo muc chan INT:
    // INT thap -> 0x5D, INT cao -> 0x14. Giu INT thap de lay 0x5D.
    pinMode(PIN_TOUCH_INT, OUTPUT);
    ioexpWrite(IOEXP_TOUCH_RST, false);
    delay(100);
    digitalWrite(PIN_TOUCH_INT, LOW);
    delay(100);
    ioexpWrite(IOEXP_TOUCH_RST, true);
    delay(200);

    // Tha INT ve input de chip dung lam chan bao co du lieu.
    pinMode(PIN_TOUCH_INT, INPUT);
    delay(50);

    uint8_t id[4] = {0};
    addr = GT911_ADDR;
    if (!readReg(REG_PRODUCT_ID, id, 4)) {
        addr = GT911_ADDR_ALT;              // vai lo hang chot o dia chi con lai
        if (!readReg(REG_PRODUCT_ID, id, 4)) return false;
    }
    for (int i = 0; i < 4; i++)
        productId[i] = (id[i] >= 32 && id[i] < 127) ? (char)id[i] : '\0';
    productId[4] = '\0';
    return true;
}

uint8_t gt911Read(TouchPoint *out, uint8_t maxPoints) {
    static TouchPoint cached[GT911_MAX_POINTS];
    static uint8_t cachedCount;
    static uint32_t lastFrame;
    uint8_t status = 0;
    if (!readReg(REG_STATUS, &status, 1)) { cachedCount=0; return 0; }
    if (!(status & 0x80)) {
        if(millis()-lastFrame>300) cachedCount=0;
        uint8_t count=cachedCount<maxPoints?cachedCount:maxPoints;
        for(uint8_t i=0;i<count;++i) out[i]=cached[i];
        return count;
    }

    uint8_t n = status & 0x0F;
    if (n > GT911_MAX_POINTS) n = GT911_MAX_POINTS;
    if (n > maxPoints) n = maxPoints;

    for (uint8_t i = 0; i < n; i++) {
        uint8_t d[8];
        if (!readReg(REG_POINT1 + i * 8, d, 8)) { n = i; break; }
        out[i].x    = (uint16_t)(d[1] | (d[2] << 8));
        out[i].y    = (uint16_t)(d[3] | (d[4] << 8));
        out[i].size = (uint16_t)(d[5] | (d[6] << 8));
    }

    // Bat buoc xoa co, khong xoa thi chip khong bao diem moi nua.
    writeReg(REG_STATUS, 0);
    lastFrame=millis(); cachedCount=n;
    for(uint8_t i=0;i<n;++i) cached[i]=out[i];
    return n;
}

const char *gt911ProductId() { return productId; }
uint8_t     gt911Address()   { return addr; }
