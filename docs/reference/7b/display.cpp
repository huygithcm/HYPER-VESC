#include "display.h"

#include <Arduino.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_idf_version.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>

#include "board_7b.h"

static esp_lcd_panel_handle_t panel = nullptr;
static uint16_t              *fb    = nullptr;
static uint16_t              *frameBuffers[2] = {};
static uint8_t                drawFbIndex = 1;

static volatile uint32_t vsyncFrames   = 0;
static volatile uint32_t vsyncMaxGapUs = 0;
static volatile int64_t  vsyncLastUs   = 0;
static SemaphoreHandle_t vsyncSem      = nullptr;
static SemaphoreHandle_t frameDoneSem  = nullptr;

// Chay trong ngat cua panel. Chi cong don, khong log - log trong ngat la treo.
static bool IRAM_ATTR onVsync(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *,
                              void *) {
    int64_t now = esp_timer_get_time();
    if (vsyncLastUs) {
        uint32_t gap = (uint32_t)(now - vsyncLastUs);
        if (gap > vsyncMaxGapUs) vsyncMaxGapUs = gap;
    }
    vsyncLastUs = now;
    vsyncFrames = vsyncFrames + 1;  // khong dung ++ tren volatile (C++20 bo)

    BaseType_t woke = pdFALSE;
    if (vsyncSem) xSemaphoreGiveFromISR(vsyncSem, &woke);
    return woke == pdTRUE;
}

static bool IRAM_ATTR onFrameComplete(esp_lcd_panel_handle_t,
                                      const esp_lcd_rgb_panel_event_data_t *,
                                      void *) {
    if (!frameDoneSem) return false;
    BaseType_t woke = pdFALSE;
    xSemaphoreGiveFromISR(frameDoneSem, &woke);
    return woke == pdTRUE;
}

bool displayWaitVsync(uint32_t timeoutMs) {
    if (!vsyncSem) return false;
    xSemaphoreTake(vsyncSem, 0);  // bo vsync cu con ton dong, doi cai KE TIEP
    return xSemaphoreTake(vsyncSem, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void displayVsyncStats(uint32_t *frames, uint32_t *maxGapUs) {
    *frames        = vsyncFrames;
    *maxGapUs      = vsyncMaxGapUs;
    vsyncFrames    = 0;
    vsyncMaxGapUs  = 0;
}

bool displayBegin() {
    esp_lcd_rgb_panel_config_t cfg = {};
    cfg.clk_src   = LCD_CLK_SRC_DEFAULT;
    cfg.data_width     = 16;
    cfg.bits_per_pixel = 16;
    cfg.num_fbs             = LCD_NUM_FB;
    cfg.bounce_buffer_size_px = LCD_BOUNCE_PX;
    // Hai truong nay bi thay bang dma_burst_size tu IDF 5.3 tro di.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
    cfg.dma_burst_size = 64;
#else
    cfg.sram_trans_align    = 4;
    cfg.psram_trans_align   = 64;
#endif

    cfg.timings.pclk_hz           = LCD_PCLK_HZ;
    cfg.timings.h_res             = LCD_W;
    cfg.timings.v_res             = LCD_H;
    cfg.timings.hsync_pulse_width = LCD_HSYNC_PULSE;
    cfg.timings.hsync_back_porch  = LCD_HSYNC_BACK;
    cfg.timings.hsync_front_porch = LCD_HSYNC_FRONT;
    cfg.timings.vsync_pulse_width = LCD_VSYNC_PULSE;
    cfg.timings.vsync_back_porch  = LCD_VSYNC_BACK;
    cfg.timings.vsync_front_porch = LCD_VSYNC_FRONT;
    cfg.timings.flags.pclk_active_neg = 1;

    cfg.hsync_gpio_num = PIN_LCD_HSYNC;
    cfg.vsync_gpio_num = PIN_LCD_VSYNC;
    cfg.de_gpio_num    = PIN_LCD_DE;
    cfg.pclk_gpio_num  = PIN_LCD_PCLK;
    cfg.disp_gpio_num  = PIN_LCD_DISP;

    const int pins[16] = PINS_LCD_DATA;
    for (int i = 0; i < 16; i++) cfg.data_gpio_nums[i] = pins[i];

    cfg.flags.fb_in_psram = 1;

    if (esp_lcd_new_rgb_panel(&cfg, &panel) != ESP_OK) return false;
    void *buffers[LCD_NUM_FB] = {};
    if (esp_lcd_rgb_panel_get_frame_buffer(panel, LCD_NUM_FB,
                                           &buffers[0], &buffers[1]) != ESP_OK)
        return false;
    for (int i = 0; i < LCD_NUM_FB; i++) {
        frameBuffers[i] = (uint16_t *)buffers[i];
        if (!frameBuffers[i]) return false;
        memset(frameBuffers[i], 0, (size_t)LCD_W * LCD_H * 2);
    }
    drawFbIndex = 1;
    fb = frameBuffers[drawFbIndex];

    // Panel RGB khong bat buoc co reset; vai ban IDF tra ve NOT_SUPPORTED nen
    // khong duoc coi la loi.
    esp_lcd_panel_reset(panel);
    if (esp_lcd_panel_init(panel) != ESP_OK) return false;

    vsyncSem = xSemaphoreCreateBinary();
    frameDoneSem = xSemaphoreCreateBinary();
    if (!vsyncSem || !frameDoneSem) return false;

    esp_lcd_rgb_panel_event_callbacks_t cbs = {};
    cbs.on_vsync = onVsync;
    cbs.on_frame_buf_complete = onFrameComplete;
    esp_lcd_rgb_panel_register_event_callbacks(panel, &cbs, nullptr);
    return true;
}

uint16_t *displayBuffer() { return fb; }

bool displayPresent() {
    if (!panel || !frameDoneSem || !fb) return false;

    while (xSemaphoreTake(frameDoneSem, 0) == pdTRUE) {}
    if (esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_W, LCD_H, fb) != ESP_OK)
        return false;

    xSemaphoreTake(frameDoneSem, portMAX_DELAY);

    drawFbIndex ^= 1u;
    fb = frameBuffers[drawFbIndex];
    return true;
}

void gfxFill(uint16_t color) {
    if (!fb) return;
    // memset chi dung khi 2 byte giong nhau; con lai ghi tung word.
    if ((color & 0xFF) == (color >> 8)) {
        memset(fb, color & 0xFF, (size_t)LCD_W * LCD_H * 2);
        return;
    }
    const size_t n = (size_t)LCD_W * LCD_H;
    for (size_t i = 0; i < n; i++) fb[i] = color;
}

void gfxRect(int x, int y, int w, int h, uint16_t color) {
    if (!fb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_W) w = LCD_W - x;
    if (y + h > LCD_H) h = LCD_H - y;
    if (w <= 0 || h <= 0) return;

    for (int row = 0; row < h; row++) {
        uint16_t *p = fb + (size_t)(y + row) * LCD_W + x;
        for (int col = 0; col < w; col++) p[col] = color;
    }
}

void gfxFrame(int x, int y, int w, int h, uint16_t color) {
    gfxRect(x, y, w, 1, color);
    gfxRect(x, y + h - 1, w, 1, color);
    gfxRect(x, y, 1, h, color);
    gfxRect(x + w - 1, y, 1, h, color);
}

int gfxTextWidth(const Font &f, const char *s) {
    return (int)strlen(s) * f.w;
}

void gfxText(const Font &f, int x, int y, const char *s, uint16_t color) {
    if (!fb) return;
    for (; *s; s++, x += f.w) {
        uint8_t c = (uint8_t)*s;
        if (c < f.first || c >= f.first + f.count) continue;
        const uint8_t *g = f.bits + (size_t)(c - f.first) * f.stride * f.h;

        for (int row = 0; row < f.h; row++) {
            int py = y + row;
            if (py < 0 || py >= LCD_H) continue;
            const uint8_t *line = g + (size_t)row * f.stride;
            uint16_t      *dst  = fb + (size_t)py * LCD_W;

            for (int col = 0; col < f.w; col++) {
                if (!(line[col >> 3] & (0x80 >> (col & 7)))) continue;
                int px = x + col;
                if (px < 0 || px >= LCD_W) continue;
                dst[px] = color;
            }
        }
    }
}

void gfxBlit(int x, int y, int w, int h, const uint16_t *src) {
    if (!fb || !src) return;
    for (int row = 0; row < h; row++) {
        int py = y + row;
        if (py < 0 || py >= LCD_H) continue;
        const uint16_t *s = src + (size_t)row * w;
        uint16_t       *d = fb + (size_t)py * LCD_W;
        for (int col = 0; col < w; col++) {
            int px = x + col;
            if (px < 0 || px >= LCD_W) continue;
            d[px] = s[col];
        }
    }
}

void gfxBlitScaled(int dx, int dy, int dw, int dh,
                   const uint16_t *src, int sw, int sh) {
    if (!fb || !src || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return;

    // Buoc co dinh dang 16.16 -> khong dung chia trong vong lap.
    const uint32_t stepX = ((uint32_t)sw << 16) / dw;
    const uint32_t stepY = ((uint32_t)sh << 16) / dh;

    uint32_t sy = 0;
    for (int row = 0; row < dh; row++, sy += stepY) {
        int py = dy + row;
        if (py < 0 || py >= LCD_H) continue;
        const uint16_t *srow = src + (size_t)(sy >> 16) * sw;
        uint16_t       *d    = fb + (size_t)py * LCD_W;

        uint32_t sx = 0;
        for (int col = 0; col < dw; col++, sx += stepX) {
            int px = dx + col;
            if (px < 0 || px >= LCD_W) continue;
            d[px] = srow[sx >> 16];
        }
    }
}
