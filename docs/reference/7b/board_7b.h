// Thong so PHAN CUNG cua Waveshare ESP32-S3-Touch-LCD-7B.
//
// Moi con so o day doc tu ma nguon chinh chu cua Waveshare:
//   examples/ESP-IDF/17_lvgl_v9_demo/components/rgb_lcd_port/rgb_lcd_port.h
//   examples/ESP-IDF/17_lvgl_v9_demo/components/io_extension/io_extension.h
// Khong tu doan - sai timing thi man chi hien nhieu hoac trang.
#pragma once

// ------------------------------------------------------------------ Panel ---
#define LCD_W 1024
#define LCD_H 600

// PCLK 30 MHz -> (1024+162+152+48) x (600+45+13+3) = 1386 x 661 = 916.146 px
// => refresh 32,7 Hz, dung theo cau hinh Waveshare chinh thuc.
// Neu man nhap nhay / xe hinh: ha xuong 16000000 truoc khi di tim loi khac.
#define LCD_PCLK_HZ 30000000

#define LCD_HSYNC_PULSE 162
#define LCD_HSYNC_BACK  152
#define LCD_HSYNC_FRONT 48
#define LCD_VSYNC_PULSE 45
#define LCD_VSYNC_BACK  13
#define LCD_VSYNC_FRONT 3

// 1 framebuffer = 1024*600*2 = 1,17 MB PSRAM. Hai framebuffer cho phep ve
// vao buffer khong dang duoc panel quet, roi doi buffer o cuoi mot frame.
#define LCD_NUM_FB 2

// Bounce buffer nam o SRAM noi (DMA), bien truy cap PSRAM ngau nhien thanh doc
// burst. Khong co no thi panel 1024x600 tren S3 gan nhu chac chan nhieu hinh.
//
// Driver cap phat HAI bo dem nay -> 1024*20 tuc la 80 KB RAM noi. Cua so
// 20 dong cho ISR du thoi gian nap lai tu PSRAM trong luc CPU copy frame.
#define LCD_BOUNCE_PX (LCD_W * 20)

#define PIN_LCD_VSYNC 3
#define PIN_LCD_HSYNC 46
#define PIN_LCD_DE    5
#define PIN_LCD_PCLK  7
#define PIN_LCD_DISP  -1

// Thu tu chan du lieu RGB565: D0..D4 = B, D5..D10 = G, D11..D15 = R
#define PINS_LCD_DATA { 14, 38, 18, 17, 10, 39, 0, 45, 48, 47, 21, 1, 2, 42, 41, 40 }

// -------------------------------------------------------------------- I2C ---
// Dung chung cho GT911 (cam ung) va IO expander.
#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9
#define I2C_HZ      400000

// ----------------------------------------------------------- IO expander ---
// Board het sach chan GPIO nen den nen / reset LCD / reset touch / CS the SD
// deu nam sau con expander nay. Giao thuc: ghi 2 byte {thanh_ghi, gia_tri}.
#define IOEXP_ADDR   0x24
#define IOEXP_R_MODE 0x02  // 1 bit / chan: 1 = output
#define IOEXP_R_OUT  0x03
#define IOEXP_R_IN   0x04
#define IOEXP_R_PWM  0x05  // do sang den nen, 0..255
#define IOEXP_R_ADC  0x06

#define IOEXP_TOUCH_RST 1  // IO1
#define IOEXP_BACKLIGHT 2  // IO2
#define IOEXP_LCD_RST   3  // IO3
#define IOEXP_SD_CS     4  // IO4
#define IOEXP_CAN_SEL   5  // IO5: 0 = USB, 1 = CAN

// ------------------------------------------------------------------ Touch ---
// GT911 chon dia chi luc reset theo muc chan INT: giu INT thap -> 0x5D.
#define PIN_TOUCH_INT 4
#define GT911_ADDR    0x5D
#define GT911_ADDR_ALT 0x14

// ------------------------------------------------------------------- Mau ---
#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

// Bang mau tin hieu - phai khop LED_MAP trong python/bambu_monitor.py.
#define C_BG      RGB565(0x0e, 0x12, 0x18)
#define C_PANEL   RGB565(0x1b, 0x21, 0x2a)
#define C_PANEL2  RGB565(0x23, 0x2b, 0x36)
#define C_LINE    RGB565(0x33, 0x3d, 0x4b)
#define C_TEXT    RGB565(0xe6, 0xec, 0xf3)
#define C_MUTED   RGB565(0x8a, 0x97, 0xa8)
#define C_DIM     RGB565(0x5d, 0x6a, 0x7b)

#define C_RUN     RGB565(0x3d, 0x7b, 0xff)
#define C_PAUSE   RGB565(0xff, 0x8c, 0x00)
#define C_ERROR   RGB565(0xff, 0x44, 0x38)
#define C_DONE    RGB565(0x26, 0xc2, 0x81)
#define C_IDLE    RGB565(0x7c, 0x88, 0x99)
#define C_SEL     RGB565(0xff, 0xd2, 0x4a)
