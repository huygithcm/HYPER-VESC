/* Windows host for the 7B review UI. No serial, CAN or BLE access. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "win32drv.h"
#include "preview_ui.h"

static void null_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color) {
    (void)area; (void)color; lv_disp_flush_ready(drv);
}

/* Render actual LVGL objects to review artifacts, not a separate mockup. */
static int export_screen(const char *path) {
    lv_obj_update_layout(lv_scr_act());
    lv_img_dsc_t *img = lv_snapshot_take(lv_scr_act(), LV_IMG_CF_TRUE_COLOR);
    if (!img) return 1;
    FILE *file = fopen(path, "wb");
    if (!file) { lv_snapshot_free(img); return 1; }
    BITMAPFILEHEADER header = {0};
    BITMAPINFOHEADER info = {0};
    const int stride = (img->header.w * 3 + 3) & ~3;
    header.bfType = 0x4d42;
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize = header.bfOffBits + stride * img->header.h;
    info.biSize = sizeof(info);
    info.biWidth = img->header.w; info.biHeight = -(LONG)img->header.h;
    info.biPlanes = 1; info.biBitCount = 24; info.biCompression = BI_RGB;
    fwrite(&header, sizeof(header), 1, file); fwrite(&info, sizeof(info), 1, file);
    const lv_color_t *pixels = (const lv_color_t *)img->data;
    for (unsigned y = 0; y < img->header.h; ++y) {
        for (unsigned x = 0; x < img->header.w; ++x) {
            lv_color32_t c; c.full = lv_color_to32(pixels[y * img->header.w + x]);
            unsigned char bgr[] = {c.ch.blue, c.ch.green, c.ch.red};
            fwrite(bgr, 3, 1, file);
        }
        for (int p = img->header.w * 3; p < stride; ++p) fputc(0, file);
    }
    fclose(file); lv_snapshot_free(img); return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show) {
    (void)previous; (void)show;
    const bool review = strstr(command, "--review") != NULL;
    lv_init();
    if (review) {
        static lv_color_t pixels[1024 * 20];
        static lv_disp_draw_buf_t buf;
        static lv_disp_drv_t drv;
        lv_disp_draw_buf_init(&buf, pixels, NULL, 1024 * 20);
        lv_disp_drv_init(&drv); drv.hor_res = 1024; drv.ver_res = 600;
        drv.draw_buf = &buf; drv.flush_cb = null_flush;
        lv_disp_drv_register(&drv);
    } else if (!lv_win32_init(instance, SW_SHOW, 1024, 600, NULL)) return 1;
    preview_ui_init();
    if (review) {
        int errors = preview_ui_self_test();
        lv_obj_t *dashboard = lv_scr_act();
        preview_ui_startup();
        lv_tick_inc(900); lv_timer_handler();
        errors += export_screen("startup-welcome.bmp");
        lv_tick_inc(1400); lv_timer_handler();
        if(lv_scr_act() != dashboard) errors++;
        preview_ui_page(PREVIEW_DASH);
        preview_ui_scenario(PREVIEW_PARK);
        errors += export_screen("dashboard-park.bmp");
        preview_ui_scenario(PREVIEW_DRIVE_2);
        errors += export_screen("dashboard-drive.bmp");
        preview_ui_scenario(PREVIEW_REVERSE);
        errors += export_screen("dashboard-reverse.bmp");
        preview_ui_scenario(PREVIEW_STALE);
        errors += export_screen("dashboard-stale.bmp");
        preview_ui_scenario(PREVIEW_FAULT);
        errors += export_screen("dashboard-fault.bmp");
        preview_ui_scenario(PREVIEW_PARK);
        preview_ui_page(PREVIEW_MODES);
        errors += export_screen("ride-modes.bmp");
        preview_ui_page(PREVIEW_BMS);
        preview_ui_bms_tab(0);
        errors += export_screen("bms-overview.bmp");
        preview_ui_bms_tab(1);
        errors += export_screen("bms-cells.bmp");
        preview_ui_bms_tab(2);
        errors += export_screen("bms-wires.bmp");
        preview_ui_page(PREVIEW_SETTINGS);
        errors += export_screen("settings.bmp");
        FILE *report = fopen("review-result.txt", "w");
        if (report) { fprintf(report, "UI self-test and render errors: %d\n", errors); fclose(report); }
        return errors ? 1 : 0;
    }
    preview_ui_startup();
    DWORD last = GetTickCount(), replay = last;
    while (!lv_win32_quit_signal) {
        DWORD now = GetTickCount(); lv_tick_inc(now - last); last = now;
        lv_timer_handler(); Sleep(5);
        if(strstr(command, "--startup-demo") && now - replay >= 5500) {
            preview_ui_startup(); replay = now;
        }
    }
    return 0;
}
