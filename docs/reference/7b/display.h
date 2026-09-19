// Panel RGB + vai ham ve toi thieu, ve thang vao framebuffer o PSRAM.
// Chua dung LVGL: buoc 1 chi can chung minh panel/timing/PSRAM dung da.
#pragma once

#include <stdint.h>

#include "font_data.h"

bool      displayBegin();
uint16_t *displayBuffer();

// Do "nhay hinh" bang so, thay vi doan bang mat.
//
// Panel quet deu 26,2 Hz => hai vsync cach nhau 38,2 ms. Bounce buffer duoc do
// day trong ngat; ngat do nam o FLASH (CONFIG_LCD_RGB_ISR_IRAM_SAFE khong bat
// trong ban Arduino nay), nen moi lan cache flash bi tat - ghi NVS chang han -
// hoac CPU bi chiem qua lau, no khong chay kip va man hinh nhay mot cai.
// Nhung lan do hien ra thanh mot khoang cach vsync dai bat thuong.
//
// Doc xong thi bo dem tu ve 0, de moi lan goi la thong ke cua ky vua roi.
void displayVsyncStats(uint32_t *frames, uint32_t *maxGapUs);

// Doi den dau khung quet ke tiep. Tra ve false neu qua han.
//
// Hien tai co HAI framebuffer (LCD_NUM_FB = 2), nen ham nay chi dung de do
// khoang cach VSYNC; viec doi buffer duoc displayPresent() lam sau mot frame.
bool displayWaitVsync(uint32_t timeoutMs);

bool displayPresent();

void gfxFill(uint16_t color);
void gfxRect(int x, int y, int w, int h, uint16_t color);
void gfxFrame(int x, int y, int w, int h, uint16_t color);   // vien 1 px
void gfxText(const Font &f, int x, int y, const char *s, uint16_t color);
int  gfxTextWidth(const Font &f, const char *s);

// Ve anh RGB565 1:1.
void gfxBlit(int x, int y, int w, int h, const uint16_t *src);

// Ve anh RGB565 co co giai (nearest neighbour). Dung cho khung camera
// 1280x720 va anh khay 512x512 -> thu nho vao o tren man hinh.
void gfxBlitScaled(int dx, int dy, int dw, int dh,
                   const uint16_t *src, int sw, int sh);
