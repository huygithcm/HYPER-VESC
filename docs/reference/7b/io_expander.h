// Con IO expander I2C giu den nen, reset LCD, reset touch va CS the SD.
// Khong dieu khien duoc no thi man hinh khong bao gio sang.
#pragma once

#include <stdint.h>

// Dat toan bo 8 chan ve output. Goi sau khi Wire.begin().
bool ioexpBegin();

// pin 0..7. Ghi ca byte moi lan (chip khong cho set tung bit).
void ioexpWrite(uint8_t pin, bool high);

// Do sang den nen 0..100 %. Waveshare chan tren o 97 de man khong tat han.
void ioexpBacklight(uint8_t percent);

// Trang thai output dang giu, de in ra serial khi go loi.
uint8_t ioexpShadow();
