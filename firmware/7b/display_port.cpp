// RGB timings/pins based on docs/reference/7b (bambu-p1s-led 2ec4600).
#include "display_port.h"
#include "board_7b.h"
#include "gt911.h"
#include <Arduino.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_heap_caps.h>
#include <freertos/semphr.h>
#include "lvgl.h"

static esp_lcd_panel_handle_t panel;
static SemaphoreHandle_t frame_done;
static uint32_t frames, timeouts, touches;
static bool IRAM_ATTR completed(esp_lcd_panel_handle_t,
        const esp_lcd_rgb_panel_event_data_t *, void *) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(frame_done, &wake);
    return wake == pdTRUE;
}
static void flush(lv_disp_drv_t *drv, const lv_area_t *, lv_color_t *pixels) {
    // LVGL's vendored direct-mode renderer synchronizes dirty areas between
    // its two full-size buffers. Present only after the final dirty rectangle.
    if(!lv_disp_flush_is_last(drv)) {lv_disp_flush_ready(drv);return;}
    while (xSemaphoreTake(frame_done, 0) == pdTRUE) {}
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_W, LCD_H, pixels));
    // Never release LVGL's old scanout buffer before the panel completes.
    if (xSemaphoreTake(frame_done, pdMS_TO_TICKS(250)) != pdTRUE) {
        ++timeouts;
        Serial.println("FATAL: LCD frame completion timeout");
        abort();
    }
    ++frames;
    lv_disp_flush_ready(drv);
}
static void touch(lv_indev_drv_t *, lv_indev_data_t *data) {
    TouchPoint p;
    if (gt911Read(&p, 1) && p.x < LCD_W && p.y < LCD_H) {
        data->point.x = p.x; data->point.y = p.y;
        data->state = LV_INDEV_STATE_PR; ++touches;
    } else data->state = LV_INDEV_STATE_REL;
}
bool display_port_begin() {
    frame_done = xSemaphoreCreateBinary();
    if (!frame_done) return false;
    esp_lcd_rgb_panel_config_t c = {};
    c.clk_src = LCD_CLK_SRC_DEFAULT; c.data_width = 16; c.bits_per_pixel = 16;
    c.num_fbs = 2; c.bounce_buffer_size_px = LCD_BOUNCE_PX; c.dma_burst_size = 64;
    c.timings.pclk_hz = LCD_PCLK_HZ;
    c.timings.h_res = LCD_W; c.timings.v_res = LCD_H;
    c.timings.hsync_pulse_width = LCD_HSYNC_PULSE;
    c.timings.hsync_back_porch = LCD_HSYNC_BACK;
    c.timings.hsync_front_porch = LCD_HSYNC_FRONT;
    c.timings.vsync_pulse_width = LCD_VSYNC_PULSE;
    c.timings.vsync_back_porch = LCD_VSYNC_BACK;
    c.timings.vsync_front_porch = LCD_VSYNC_FRONT;
    c.timings.flags.pclk_active_neg = 1;
    c.hsync_gpio_num = PIN_LCD_HSYNC; c.vsync_gpio_num = PIN_LCD_VSYNC;
    c.de_gpio_num = PIN_LCD_DE; c.pclk_gpio_num = PIN_LCD_PCLK; c.disp_gpio_num = -1;
    const int pins[] = PINS_LCD_DATA;
    for (int i=0; i<16; ++i) c.data_gpio_nums[i]=pins[i];
    c.flags.fb_in_psram = 1;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&c, &panel));
    void *a, *b;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel, 2, &a, &b));
    memset(a,0,LCD_W*LCD_H*2); memset(b,0,LCD_W*LCD_H*2);
    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_frame_buf_complete = completed;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel,&callbacks,nullptr));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    static lv_disp_draw_buf_t buffers;
    lv_disp_draw_buf_init(&buffers,a,b,LCD_W*LCD_H);
    static lv_disp_drv_t drv; lv_disp_drv_init(&drv);
    drv.hor_res=LCD_W; drv.ver_res=LCD_H; drv.direct_mode=1;
    drv.draw_buf=&buffers; drv.flush_cb=flush;
    if (!lv_disp_drv_register(&drv)) return false;
    static lv_indev_drv_t input; lv_indev_drv_init(&input);
    input.type=LV_INDEV_TYPE_POINTER; input.read_cb=touch;
    if (!lv_indev_drv_register(&input)) return false;
    return true;
}
void display_port_stats() {
    Serial.printf("HEALTH uptime=%lu frames=%lu touch=%lu timeouts=%lu heap=%u psram=%u\n",
        millis(),frames,touches,timeouts,heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
