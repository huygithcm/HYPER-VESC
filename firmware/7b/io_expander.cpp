#include "io_expander.h"

#include <Wire.h>

#include "board_7b.h"

// Waveshare khoi tao shadow = 0xFF nhung khong day xuong chip ngay; lan
// ioexpWrite dau tien moi ghi ca byte. Giu nguyen hanh vi do: nghia la ngay
// khi cham vao chan dau tien thi den nen (IO2) va CAN_SEL (IO5) len muc cao.
static uint8_t shadow = 0xFF;

static bool writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(IOEXP_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool ioexpBegin() {
    shadow = 0xFF;
    return writeReg(IOEXP_R_MODE, 0xFF);  // tat ca la output
}

void ioexpWrite(uint8_t pin, bool high) {
    if (high)
        shadow |= (uint8_t)(1u << pin);
    else
        shadow &= (uint8_t)~(1u << pin);
    writeReg(IOEXP_R_OUT, shadow);
}

void ioexpBacklight(uint8_t percent) {
    if (percent > 97) percent = 97;  // 100% lam chip tat han den, khong phai sang nhat
    writeReg(IOEXP_R_PWM, (uint8_t)(percent * 255 / 100));
}

uint8_t ioexpShadow() { return shadow; }
