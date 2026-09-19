// Driver toi thieu cho cam ung GT911 - chi doc diem cham.
// Viet tay thay vi keo component esp_lcd_touch_gt911 vi component do la cua
// ESP-IDF, keo vao du an Arduino rat phien.
#pragma once

#include <stdint.h>

#define GT911_MAX_POINTS 5

struct TouchPoint {
    uint16_t x, y, size;
};

// Chay chuoi reset (qua IO expander) roi bat tay voi chip.
// Tra ve false neu khong doc duoc product id.
bool gt911Begin();

// So diem dang cham, ghi vao out[]. Tra ve 0 neu khong co.
uint8_t gt911Read(TouchPoint *out, uint8_t maxPoints);

// "911" doc duoc luc begin, de in ra serial.
const char *gt911ProductId();

// Dia chi I2C thuc su bat tay duoc (0x5D hoac 0x14).
uint8_t gt911Address();
