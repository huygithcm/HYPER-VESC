/* Shared 7B view. Host uses demo states; UI_7B_HARDWARE binds real backends.
 * Layout follows example_UI; fonts are Antonio from the source seed.
 */
#include "preview_ui.h"
#include "branding.h"
#include "lvgl.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef UI_7B_HARDWARE
#include "backend.h"
#include "bms/bms_model.h"
static void hardware_update(void);
static void hardware_bms_view(void);
static uint32_t config_epoch;
static bool config_loaded;
static uint16_t pending_config, pending_park;
static uint32_t pending_config_at, pending_park_at;
static int pending_mode=-1;
static uint32_t pending_mode_at, pending_mode_epoch;
#endif
static lv_obj_t *soc_label, *voltage_label, *odo_label, *trip_label, *range_label;
static lv_obj_t *battery_temp, *bars[20], *mode_heading, *reverse_limits;

LV_FONT_DECLARE(lv_font_Antonio_Regular_32);
LV_FONT_DECLARE(lv_font_Antonio_Regular_40);
LV_FONT_DECLARE(lv_font_Antonio_Regular_64);
LV_FONT_DECLARE(lv_font_Antonio_Regular_200);

#define BLACK  0x030405
#define PANEL  0x0d1013
#define WHITE  0xf4f5f5
#define MUTED  0x819098
#define LINE   0x30393e
#define ORANGE 0xff701f
#define BLUE   0x229cf2
#define GREEN  0x67df42
#define RED    0xff5656
#define F16 (&lv_font_montserrat_16)
#define F20 (&lv_font_montserrat_20)
#define F24 (&lv_font_montserrat_24)
#define A32 (&lv_font_Antonio_Regular_32)
#define A40 (&lv_font_Antonio_Regular_40)
#define A64 (&lv_font_Antonio_Regular_64)

static lv_obj_t *root, *pages[4], *nav[4], *overlay, *toast_box, *toast_text;
static lv_obj_t *speed_label, *motor_current, *gear_ring, *gear_text, *gear_name;
static lv_obj_t *drive_status, *can_status, *clock_label, *dash_note, *ble_status;
static lv_obj_t *mode_values[3], *effective_values[3], *mode_selected[3], *mode_cards[3];
static lv_obj_t *bms_body, *bms_tabs[3], *brightness_value, *target_value;
static lv_obj_t *reverse_button;
static preview_page_t current_page;
static preview_scenario_t scenario;
static int draft_amps[3] = {50, 70, 100}, saved_amps[3] = {50, 70, 100};
static int target_id = 10, brightness = 80, active_mode = 0, bms_tab;
static bool reverse_enabled, animate_demo;
static uint32_t toast_until, tick_count;
static int demo_speed;

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h,
                     uint32_t fill, uint32_t border, int radius) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y); lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(fill), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, border ? 1 : 0, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(border), 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *text(lv_obj_t *parent, const char *s, int x, int y, int w,
                      const lv_font_t *font, uint32_t color, lv_text_align_t align) {
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, s); lv_obj_set_pos(o, x, y); lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(o, align, 0);
    return o;
}
#define LEFT LV_TEXT_ALIGN_LEFT
#define CENTER LV_TEXT_ALIGN_CENTER
#define RIGHT LV_TEXT_ALIGN_RIGHT

static void rule(lv_obj_t *p, int x, int y, int w, uint32_t color) {
    box(p, x, y, w, 1, color, 0, 0);
}
/* Cubic strokes keep tangent continuity at corners. Width and opacity taper
 * along the curve, matching the fine-to-bold orange trim in example_UI. */
static void curve(lv_draw_ctx_t *ctx, const lv_area_t *a,
                  int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3,
                  int w0, int w1) {
    lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d);
    d.color = lv_color_hex(ORANGE); d.round_start = 1; d.round_end = 1;
    lv_point_t prev = {a->x1 + x0, a->y1 + y0};
    for (int i = 1; i <= 32; ++i) {
        float t = i / 32.0f, u = 1.0f - t;
        lv_point_t next = {
            a->x1 + (int)(u*u*u*x0 + 3*u*u*t*x1 + 3*u*t*t*x2 + t*t*t*x3 + .5f),
            a->y1 + (int)(u*u*u*y0 + 3*u*u*t*y1 + 3*u*t*t*y2 + t*t*t*y3 + .5f)
        };
        float width = w0 + (w1 - w0) * t;
        d.width = (int)(width + .5f);
        /* Opaque blended ink avoids bright seams where segments overlap. */
        d.opa = LV_OPA_COVER;
        d.color = lv_color_mix(lv_color_hex(ORANGE), lv_color_hex(BLACK),
                              (uint8_t)(115 + 140 * width / 6));
        lv_draw_line(ctx, &d, &prev, &next); prev = next;
    }
}
static void trim_cb(lv_event_t *e) {
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
    lv_area_t a; lv_obj_get_coords(lv_event_get_target(e), &a);
    if (lv_event_get_user_data(e)) {
        curve(ctx,&a,28,65,150,65,280,65,392,65,2,2);
        curve(ctx,&a,392,65,416,65,406,77,430,77,2,2);
        curve(ctx,&a,430,77,470,77,554,77,594,77,2,2);
        curve(ctx,&a,594,77,618,77,608,65,632,65,2,2);
        curve(ctx,&a,632,65,744,65,874,65,996,65,2,2);
    } else {
        curve(ctx,&a,64,64,40,64,34,64,34,86,1,6);
        curve(ctx,&a,34,86,34,128,34,190,34,228,6,6);
        curve(ctx,&a,34,228,34,250,40,250,70,250,6,1);
        curve(ctx,&a,598,64,622,64,628,64,628,86,1,6);
        curve(ctx,&a,628,86,628,128,628,190,628,228,6,6);
        curve(ctx,&a,628,228,628,250,622,250,598,250,6,3);
        curve(ctx,&a,598,250,475,250,352,250,232,250,3,1);
    }
}
static lv_obj_t *button(lv_obj_t *p, const char *s, int x, int y, int w, int h,
                        lv_event_cb_t cb, intptr_t arg, uint32_t accent) {
    lv_obj_t *o = box(p, x, y, w, h, PANEL, accent ? accent : LINE, 9);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(o, lv_color_hex(0x263039), LV_STATE_PRESSED);
    lv_obj_t *l = text(o, s, 0, 0, w - 4, F16, accent ? accent : WHITE, CENTER);
    lv_obj_center(l);
    if (cb) lv_obj_add_event_cb(o, cb, LV_EVENT_CLICKED, (void *)arg);
    return o;
}
static void set_button_text(lv_obj_t *button_obj, const char *s) {
    lv_label_set_text(lv_obj_get_child(button_obj, 0), s);
}
static void toast(const char *message) {
    lv_label_set_text(toast_text, message);
    lv_obj_clear_flag(toast_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast_box); toast_until = lv_tick_get() + 3500;
}
static void nav_cb(lv_event_t *e) { preview_ui_page((preview_page_t)(intptr_t)lv_event_get_user_data(e)); }
#ifdef UI_7B_HARDWARE
static void park_cb(lv_event_t *e) {
    (void)e; telemetry_t t; backend_snapshot(&t);
    vesc_ride_safety_t s={0}; uint16_t seq;
    if(!t.fresh || !vesc_ride_mode_get_safety(&s) || !s.valid) {toast("Park / Drive unavailable: stale state.");return;}
    bool queued=vesc_ride_mode_set_park(s.state!=VESC_RIDE_SAFETY_PARK,&seq);
    if(queued) {pending_park=seq;pending_park_at=lv_tick_get();}
    toast(queued ?
          "Park / Drive requested. Brake and ESC interlocks apply." : "Request unavailable. Release and try again.");
}
#endif
static void close_overlay(lv_event_t *e) {
    (void)e; if (overlay) { lv_obj_del(overlay); overlay = NULL; }
}
static void scenario_cb(lv_event_t *e) {
    preview_scenario_t next = (preview_scenario_t)(intptr_t)lv_event_get_user_data(e);
    close_overlay(NULL); preview_ui_scenario(next);
}
static void animation_cb(lv_event_t *e) {
    animate_demo = !animate_demo;
    set_button_text(lv_event_get_target(e), animate_demo ? "Animation: ON" : "Animation: OFF");
}
static void preview_cb(lv_event_t *e) {
    (void)e;
    if (overlay) return;
    overlay = box(root, 0, 0, 1024, 600, BLACK, 0, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_80, 0);
    lv_obj_t *panel = box(overlay, 202, 114, 620, 362, PANEL, LINE, 18);
    text(panel, "Preview scenarios", 24, 24, 430, F24, WHITE, LEFT);
    text(panel, "Sample states only. No vehicle commands.", 24, 63, 560, F16, MUTED, LEFT);
    button(panel, LV_SYMBOL_CLOSE, 545, 18, 50, 45, close_overlay, 0, 0);
    const char *names[] = {"Park / P", "Mode 1", "Mode 2", "Mode 3", "Reverse / R", "No CAN data", "ESC fault"};
    for (int i = 0; i < 7; ++i)
        button(panel, names[i], 24 + (i % 3) * 194, 111 + (i / 3) * 64,
               182, 52, scenario_cb, i, scenario == i ? ORANGE : 0);
    button(panel, animate_demo ? "Animation: ON" : "Animation: OFF", 218, 239,
           376, 52, animation_cb, 0, BLUE);
}

static void top_chrome(void) {
    text(root, DISPLAY_BRAND_NAME, 30, 22, 235, F24, WHITE, LEFT);
    text(root, "7B", 225, 29, 42, F16, MUTED, LEFT);
    clock_label = text(root, "12", 449, 13, 58, A40, WHITE, CENTER);
    text(root, ":", 505, 20, 14, F24, WHITE, CENTER);
    text(root,
#ifdef UI_7B_HARDWARE
         "--",
#else
         "45",
#endif
         520, 13, 58, A40, WHITE, CENTER);
    ble_status = text(root, LV_SYMBOL_BLUETOOTH, 713, 24, 28, F24, BLUE, CENTER);
    can_status = text(root, "CAN", 750, 28, 60, F16, GREEN, LEFT);
#ifdef UI_7B_HARDWARE
    lv_label_set_text(clock_label, "--");
    /* No RTC time is invented on a device without time synchronization. */
    text(root, "LIVE", 866, 24, 128, F16, ORANGE, CENTER);
#else
    button(root, "PREVIEW", 866, 15, 128, 38, preview_cb, 0, ORANGE);
#endif
    lv_obj_add_event_cb(root, trim_cb, LV_EVENT_DRAW_MAIN, (void *)1);
    const char *names[] = {LV_SYMBOL_HOME "  Dashboard", "Ride modes", LV_SYMBOL_BATTERY_FULL "  BMS", LV_SYMBOL_SETTINGS "  Settings"};
    rule(root, 28, 537, 968, LINE);
    for (int i = 0; i < 4; ++i) nav[i] = button(root, names[i], 28 + i * 244, 549, 236, 42, nav_cb, i, 0);
}

static lv_obj_t *ecu_temp, *motor_temp;
static lv_obj_t *temperature(lv_obj_t *p, const char *name, const char *value, int x, uint32_t color) {
    lv_obj_t *badge = box(p, x, 398, 36, 36, BLACK, color, 18);
    text(badge, name, 0, 9, 36, &lv_font_montserrat_12, color, CENTER);
    lv_obj_t *label = text(p, value, x + 49, 390, 91, A40, WHITE, LEFT);
    text(p, "°C", x + 135, 404, 36, F20, MUTED, LEFT);
    return label;
}

static void dashboard(void) {
    lv_obj_t *p = pages[PREVIEW_DASH];
    text(p, "SPEED", 38, 18, 160, F16, MUTED, LEFT);
    drive_status = text(p, "PARKED", 384, 18, 235, F16, ORANGE, RIGHT);
    lv_obj_add_event_cb(p, trim_cb, LV_EVENT_DRAW_MAIN, NULL);
    speed_label = text(p, "00", 169, 45, 324, &lv_font_Antonio_Regular_200, WHITE, CENTER);
    text(p, "km/h", 517, 206, 90, F24, WHITE, RIGHT);
    motor_current = text(p, "0.0", 81, 229, 104, A40, WHITE, CENTER);
    text(p, "A", 190, 247, 30, F20, WHITE, LEFT);
    dash_note = text(p, "Drive locked  /  ready in Mode 1", 35, 280, 593, F16, MUTED, CENTER);
    box(p, 34, 324, 594, 55, BLACK, 0x737a7e, 11);
    text(p, "ODO", 50, 344, 60, F16, WHITE, LEFT);
    odo_label = text(p, "1288", 121, 331, 112, A32, WHITE, RIGHT);
    text(p, "km", 241, 346, 40, F16, MUTED, LEFT);
    box(p, 331, 337, 1, 29, LINE, 0, 0);
    text(p, "TRIP", 347, 344, 60, F16, WHITE, LEFT);
    trip_label = text(p, "24.6", 418, 331, 112, A32, WHITE, RIGHT);
    text(p, "km", 538, 346, 40, F16, MUTED, LEFT);
    ecu_temp = temperature(p, "ECU", "38", 34, ORANGE);
    motor_temp = temperature(p, "M", "44", 246, BLUE);
    battery_temp = temperature(p, "BAT", "31", 457, GREEN);
    box(p, 673, 27, 1, 407, LINE, 0, 0);
    text(p, LV_SYMBOL_BATTERY_FULL, 713, 19, 48, F24, GREEN, LEFT);
    text(p, "BATTERY", 769, 23, 206, F16, MUTED, LEFT);
    soc_label = text(p, "82", 711, 55, 145, A64, WHITE, LEFT);
    text(p, "%", 798, 83, 40, F24, WHITE, LEFT);
    voltage_label = text(p, "52.6", 855, 70, 93, A40, WHITE, RIGHT);
    text(p, "V", 955, 93, 25, F20, MUTED, LEFT);
    for (int i = 0; i < 20; ++i) bars[i] = box(p, 713 + i * 13, 132, 9, 14, i < 16 ? GREEN : LINE, 0, 2);
    text(p, "RANGE", 713, 171, 116, F16, MUTED, LEFT);
    range_label = text(p, "64", 846, 162, 90, A40, WHITE, RIGHT);
    text(p, "km", 945, 184, 39, F16, MUTED, LEFT);
    rule(p, 713, 222, 263, LINE);
    gear_ring = box(p, 771, 253, 148, 148, BLACK, BLUE, 74);
    lv_obj_set_style_border_width(gear_ring, 6, 0);
#ifdef UI_7B_HARDWARE
    lv_obj_add_flag(gear_ring,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(gear_ring,park_cb,LV_EVENT_LONG_PRESSED,NULL);
#endif
    lv_obj_t *inner = box(gear_ring, 7, 7, 122, 122, BLACK, 0x18456a, 61);
    lv_obj_set_style_border_width(inner, 1, 0);
    lv_obj_clear_flag(inner,LV_OBJ_FLAG_CLICKABLE);
    gear_text = text(gear_ring, "P", 0, 0, 130, &lv_font_montserrat_48, BLUE, CENTER);
    lv_obj_center(gear_text);
    gear_name = text(p, "PARK", 713, 416, 264, F16, BLUE, CENTER);
}

static int effective(int value) { return value < 70 ? value : 70; }
static void refresh_modes(void) {
#ifdef UI_7B_HARDWARE
    vesc_ride_config_t cfg={0};
    bool valid=config_loaded && vesc_ride_mode_get_config(&cfg) && cfg.valid;
    for(int i=0;i<3;++i) {
        if(valid) {
            lv_label_set_text_fmt(mode_values[i],"%d",draft_amps[i]);
            float limit=draft_amps[i]<cfg.esc_current_max_dA/10.f?draft_amps[i]:cfg.esc_current_max_dA/10.f;
            char s[48];snprintf(s,sizeof(s),"Effective %.1f A",limit);lv_label_set_text(effective_values[i],s);
        } else {lv_label_set_text(mode_values[i],"--");lv_label_set_text(effective_values[i],"Not confirmed");}
        lv_obj_set_style_border_color(mode_cards[i],lv_color_hex(valid && active_mode==i?ORANGE:LINE),0);
        set_button_text(mode_selected[i],valid && active_mode==i?"Selected":"Select mode");
    }
    return;
#endif
    for (int i = 0; i < 3; ++i) {
        lv_label_set_text_fmt(mode_values[i], "%d", draft_amps[i]);
        lv_label_set_text_fmt(effective_values[i], "Effective %d A%s", effective(draft_amps[i]), draft_amps[i] > 70 ? "  /  ESC capped" : "");
        lv_obj_set_style_text_color(effective_values[i], lv_color_hex(draft_amps[i] > 70 ? ORANGE : MUTED), 0);
        lv_obj_set_style_border_color(mode_cards[i], lv_color_hex(active_mode == i ? ORANGE : LINE), 0);
        set_button_text(mode_selected[i], active_mode == i ? "Selected" : "Select mode");
    }
}
static void adjust_mode_cb(lv_event_t *e) {
#ifdef UI_7B_HARDWARE
    if(!config_loaded) {toast("Waiting for Lisp configuration.");return;}
#endif
    int code = (int)(intptr_t)lv_event_get_user_data(e);
    int mode = code / 2, delta = code % 2 ? 5 : -5;
    int next = draft_amps[mode] + delta;
    draft_amps[mode] = next < 1 ? 1 : next > 999 ? 999 : next;
    refresh_modes();
}
static void select_mode_cb(lv_event_t *e) {
#ifdef UI_7B_HARDWARE
    uint16_t seq;
    if (!config_loaded || scenario == PREVIEW_STALE || scenario == PREVIEW_FAULT || scenario == PREVIEW_REVERSE) {
        toast("Mode unavailable: waiting for fresh ESC / Lisp state."); return;
    }
    bool queued = vesc_ride_mode_select((uint8_t)(intptr_t)lv_event_get_user_data(e), &seq);
    if(queued) {
        vesc_ride_safety_t s={0};vesc_ride_mode_get_safety(&s);
        pending_mode=(int)(intptr_t)lv_event_get_user_data(e);
        pending_mode_at=lv_tick_get();pending_mode_epoch=s.epoch;
    }
    toast(queued ? "Mode requested. Waiting for ESC confirmation." : "Mode request unavailable.");
    return;
#endif
    if (scenario == PREVIEW_REVERSE || scenario == PREVIEW_STALE || scenario == PREVIEW_FAULT) {
        toast("Preview: mode selection unavailable in this state."); return;
    }
    active_mode = (int)(intptr_t)lv_event_get_user_data(e);
    if (scenario >= PREVIEW_DRIVE_1 && scenario <= PREVIEW_DRIVE_3)
        preview_ui_scenario((preview_scenario_t)(PREVIEW_DRIVE_1 + active_mode));
    refresh_modes(); toast("Preview mode selected. No ESC command sent.");
}
static void save_modes_cb(lv_event_t *e) {
#ifdef UI_7B_HARDWARE
    (void)e;
    vesc_ride_config_t cfg; uint16_t seq;
    if (scenario != PREVIEW_PARK || !config_loaded || !vesc_ride_mode_get_config(&cfg) || !cfg.valid) {
        toast("Save requires fresh Lisp configuration and Park."); return;
    }
    for (int i=0;i<3;++i) cfg.mode_current_dA[i] = draft_amps[i]*10;
    cfg.reverse_enabled = reverse_enabled;
    bool queued=vesc_ride_mode_set_config(&cfg,&seq);
    if(queued) {pending_config=seq;pending_config_at=lv_tick_get();}
    toast(queued ? "Save requested. Waiting for ESC confirmation." : "Save not queued.");
    return;
#endif
    (void)e;
    if (scenario != PREVIEW_PARK) { toast("Preview: stop and park before saving mode limits."); return; }
    memcpy(saved_amps, draft_amps, sizeof(saved_amps));
    toast("Preview settings saved in RAM only.");
}
static void reverse_cb(lv_event_t *e) {
    if (scenario != PREVIEW_PARK) { toast("Preview: park before changing reverse settings."); return; }
    reverse_enabled = !reverse_enabled;
    set_button_text(lv_event_get_target(e), reverse_enabled ? "Enabled" : "Disabled");
#ifdef UI_7B_HARDWARE
    toast("Staged. Press Save to send; ESC owns reverse interlocks.");
#else
    toast("Preview only. Physical RX hold / brake confirmation remains required on ESC.");
#endif
}
static void modes_page(void) {
    lv_obj_t *p = pages[PREVIEW_MODES];
    text(p, "Ride modes", 32, 18, 400, F24, WHITE, LEFT);
    mode_heading = text(p, "Motor current limit  /  ESC maximum: 70 A", 450, 25, 540, F16, MUTED, RIGHT);
    for (int i = 0; i < 3; ++i) {
        int x = 32 + 328 * i;
        mode_cards[i] = box(p, x, 69, 304, 254, PANEL, LINE, 15);
        lv_obj_t *card = mode_cards[i];
        char title[24]; snprintf(title, sizeof(title), "MODE %d", i + 1);
        text(card, title, 20, 17, 264, F16, ORANGE, LEFT);
        mode_values[i] = text(card, "50", 20, 57, 202, A64, WHITE, CENTER);
        text(card, "A", 228, 87, 40, F24, MUTED, LEFT);
        effective_values[i] = text(card, "", 16, 132, 272, F16, MUTED, CENTER);
        button(card, "-", 18, 165, 62, 42, adjust_mode_cb, i * 2, 0);
        text(card, "5 A / step", 90, 178, 122, F16, MUTED, CENTER);
        button(card, "+", 224, 165, 62, 42, adjust_mode_cb, i * 2 + 1, 0);
        mode_selected[i] = button(card, "Select mode", 18, 212, 268, 32, select_mode_cb, i, 0);
    }
    text(p, "Reverse", 34, 348, 165, F20, WHITE, LEFT);
    reverse_button = button(p, "Disabled", 203, 337, 132, 44, reverse_cb, 0, BLUE);
    reverse_limits = text(p, "3.0 km/h  /  7.0 A", 370, 350, 340, F20, MUTED, LEFT);
    text(p, "Limits are capped by the ESC. Speed limits remain in Motor Settings.", 34, 415, 724, F16, MUTED, LEFT);
    button(p,
#ifdef UI_7B_HARDWARE
        "Save to ESC",
#else
        "Save preview",
#endif
        794, 395, 198, 46, save_modes_cb, 0, ORANGE);
    refresh_modes();
}

static void bms_tab_cb(lv_event_t *e) { preview_ui_bms_tab((int)(intptr_t)lv_event_get_user_data(e)); }
#ifdef UI_7B_HARDWARE
static void choose_bms(lv_event_t *e) {
    backend_bms_select((int)(intptr_t)lv_event_get_user_data(e));
    close_overlay(NULL); toast("Connecting to selected JK BMS...");
}
static void scan_bms(lv_event_t *e) { (void)e; backend_bms_scan(); toast("Scanning for 5 seconds. Reopen Device for results."); }
#endif
static void pair_cb(lv_event_t *e) {
    (void)e;
#ifdef UI_7B_HARDWARE
    if(overlay) return;
    overlay=box(root,0,0,1024,600,BLACK,0,0);
    text(overlay,"JK BMS devices",32,28,650,F24,WHITE,LEFT);
    button(overlay,"Scan",730,20,120,44,scan_bms,0,ORANGE);
    button(overlay,"Close",866,20,126,44,close_overlay,0,0);
    char names[6][48]; int n=backend_bms_devices(names);
    if(!n) text(overlay,"Press Scan, then reopen Device to choose a nearby JK BMS.",32,100,950,F20,MUTED,LEFT);
    for(int i=0;i<n;++i) button(overlay,names[i],32,90+i*76,960,60,choose_bms,i,BLUE);
#else
    toast("Preview device: JK-B2A24S  /  BLE radio is not used.");
#endif
}
static void stat(lv_obj_t *parent, const char *name, const char *value, const char *unit,
                 int x, int y, int w, uint32_t color) {
    lv_obj_t *card = box(parent, x, y, w, 130, PANEL, LINE, 12);
    text(card, name, 20, 17, w - 40, F16, MUTED, LEFT);
    text(card, value, 20, 51, w - 100, A40, color, LEFT);
    text(card, unit, w - 78, 77, 62, F20, MUTED, LEFT);
}
void preview_ui_bms_tab(int tab) {
    if (tab < 0 || tab > 2) return;
    bms_tab = tab; lv_obj_clean(bms_body);
    for (int i = 0; i < 3; ++i)
        lv_obj_set_style_border_color(bms_tabs[i], lv_color_hex(i == tab ? ORANGE : LINE), 0);
#ifdef UI_7B_HARDWARE
    hardware_bms_view();
    return;
#endif
    if (tab == 0) {
        stat(bms_body, "STATE OF CHARGE", "82", "%", 32, 10, 304, GREEN);
        stat(bms_body, "PACK VOLTAGE", "52.6", "V", 360, 10, 304, WHITE);
        stat(bms_body, "DISCHARGE CURRENT", "0.0", "A", 688, 10, 304, WHITE);
        stat(bms_body, "REMAINING", "24.6", "Ah", 32, 164, 304, WHITE);
        stat(bms_body, "CELL DELTA", "12", "mV", 360, 164, 304, BLUE);
        stat(bms_body, "MOS TEMPERATURE", "31", "°C", 688, 164, 304, WHITE);
        text(bms_body, "16 cells   /   30 Ah   /   Charge ON   /   Discharge ON", 34, 325, 780, F16, MUTED, LEFT);
    } else {
        for (int i = 0; i < 16; ++i) {
            int x = 32 + (i % 8) * 122, y = 14 + (i / 8) * 143;
            lv_obj_t *cell = box(bms_body, x, y, 110, 126, PANEL, i == 7 ? BLUE : LINE, 9);
            char title[20]; snprintf(title, sizeof(title), "%s %02d", tab == 1 ? "CELL" : "WIRE", i + 1);
            text(cell, title, 9, 13, 92, &lv_font_montserrat_12, MUTED, CENTER);
            char value[24];
            if (tab == 1) snprintf(value, sizeof(value), "%.3f", 3.281 + (i % 7) * 0.002);
            else snprintf(value, sizeof(value), "%.1f", 0.6 + (i % 4) * 0.1);
            text(cell, value, 7, 40, 96, A32, WHITE, CENTER);
            text(cell, tab == 1 ? "V" : "mOhm", 7, 83, 96, &lv_font_montserrat_12, MUTED, CENTER);
            box(cell, 13, 111, 84, 3, tab == 1 ? GREEN : BLUE, 0, 1);
        }
        text(bms_body, tab == 1 ? "Minimum 3.281 V    /    Maximum 3.293 V    /    Delta 12 mV" :
             "Wire resistance from the BMS settings frame. Sample data.",
             34, 324, 950, F16, MUTED, LEFT);
    }
}
static void bms_page(void) {
    lv_obj_t *p = pages[PREVIEW_BMS];
    text(p, "JK BMS", 32, 18, 180, F24, WHITE, LEFT);
    text(p,
#ifdef UI_7B_HARDWARE
         "Bluetooth / JK BMS",
#else
         "JK-B2A24S  /  sample",
#endif
         32, 52, 300, F16, MUTED, LEFT);
    const char *names[] = {"Overview", "Cells", "Wires"};
    for (int i = 0; i < 3; ++i) bms_tabs[i] = button(p, names[i], 463 + i * 131, 15, 120, 42, bms_tab_cb, i, 0);
    button(p, "Device", 866, 15, 126, 42, pair_cb, 0, BLUE);
    bms_body = box(p, 0, 83, 1024, 366, BLACK, 0, 0);
    preview_ui_bms_tab(0);
}

static void brightness_cb(lv_event_t *e) {
    brightness = lv_slider_get_value(lv_event_get_target(e));
    lv_label_set_text_fmt(brightness_value, "%d%%", brightness);
#ifdef UI_7B_HARDWARE
    backend_set_brightness(brightness);
#endif
}
static void target_cb(lv_event_t *e) {
    int next = target_id + (int)(intptr_t)lv_event_get_user_data(e);
#ifdef UI_7B_HARDWARE
    if(next<0 || next>253) return;
#endif
    if (next >= 0 && next <= 253) target_id = next;
    lv_label_set_text_fmt(target_value, "%d", target_id);
#ifdef UI_7B_HARDWARE
    toast(backend_set_target(target_id) ? "Target saved. Restart display to apply." : "Target must be 0..253; 254 is this display.");
#else
    toast("Preview target changed. No CAN connection.");
#endif
}
static void reset_cb(lv_event_t *e) {
    (void)e; target_id = 10;
    lv_label_set_text(target_value, "10");
    draft_amps[0] = saved_amps[0] = 50; draft_amps[1] = saved_amps[1] = 70; draft_amps[2] = saved_amps[2] = 100;
    reverse_enabled = false; set_button_text(reverse_button, "Disabled");
    refresh_modes(); preview_ui_scenario(PREVIEW_PARK);
    toast("Preview ride settings reset. Hardware configuration was not changed.");
}
static void settings_page(void) {
    lv_obj_t *p = pages[PREVIEW_SETTINGS];
    text(p, "Settings", 32, 18, 500, F24, WHITE, LEFT);
    text(p, "Waveshare ESP32-S3 7B", 617, 25, 375, F16, MUTED, RIGHT);
    lv_obj_t *display = box(p, 32, 78, 464, 166, PANEL, LINE, 14);
    text(display, "DISPLAY", 20, 19, 390, F16, ORANGE, LEFT);
    text(display, "Brightness", 20, 57, 290, F20, WHITE, LEFT);
    brightness_value = text(display, "80%", 338, 57, 104, F20, WHITE, RIGHT);
    lv_label_set_text_fmt(brightness_value,"%d%%",brightness);
    lv_obj_t *slider = lv_slider_create(display);
    lv_obj_set_pos(slider, 30, 123); lv_obj_set_size(slider, 400, 8);
    lv_slider_set_range(slider, 10, 100); lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
#ifdef UI_7B_HARDWARE
    lv_slider_set_range(slider,10,97);
#endif
    lv_obj_set_style_bg_color(slider, lv_color_hex(ORANGE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(WHITE), LV_PART_KNOB);
    lv_obj_add_event_cb(slider, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *can = box(p, 528, 78, 464, 166, PANEL, LINE, 14);
    text(can, "VESC CONNECTION", 20, 19, 390, F16, ORANGE, LEFT);
    text(can, "Target ID", 20, 66, 178, F20, WHITE, LEFT);
    button(can, "-", 222, 51, 56, 48, target_cb, -1, 0);
    target_value = text(can, "10", 283, 59, 87, F24, WHITE, CENTER);
    lv_label_set_text_fmt(target_value,"%d",target_id);
    button(can, "+", 378, 51, 56, 48, target_cb, 1, 0);
    text(can, "CAN baudrate   1000 kbit/s", 20, 126, 424, F16, MUTED, LEFT);
    lv_obj_t *info = box(p, 32, 265, 960, 105, PANEL, LINE, 14);
    text(info, "DISPLAY HARDWARE", 20, 17, 450, F16, MUTED, LEFT);
    text(info, "1024 x 600   /   16 MB flash   /   8 MB PSRAM", 20, 55, 910, F20, WHITE, LEFT);
#ifdef UI_7B_HARDWARE
    text(p, "Brightness saved automatically. CAN target applies after restart.", 34, 416, 950, F16, MUTED, LEFT);
#else
    text(p, "Preview values live in RAM. Nothing is written to the display or VESC.", 34, 416, 716, F16, MUTED, LEFT);
    button(p, "Reset preview", 794, 395, 198, 46, reset_cb, 0, ORANGE);
#endif
}

void preview_ui_page(preview_page_t page) {
    if (page < PREVIEW_DASH || page > PREVIEW_SETTINGS) return;
    current_page = page;
#ifdef UI_7B_HARDWARE
    vesc_ride_mode_set_screen_active(page == PREVIEW_MODES);
#endif
    for (int i = 0; i < 4; ++i) {
        if (page == i) lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_border_color(nav[i], lv_color_hex(page == i ? ORANGE : LINE), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(nav[i], 0), lv_color_hex(page == i ? ORANGE : MUTED), 0);
    }
}
void preview_ui_scenario(preview_scenario_t state) {
    if (state < PREVIEW_PARK || state > PREVIEW_FAULT) return;
    scenario = state;
    const char *gear = "P", *status = "PARKED", *note = "Drive locked  /  parked", *name = "PARK";
    uint32_t color = BLUE;
    demo_speed = 0;
    if (state >= PREVIEW_DRIVE_1 && state <= PREVIEW_DRIVE_3) {
        active_mode = state - PREVIEW_DRIVE_1;
        static const char *gears[] = {"1", "2", "3"};
        gear = gears[active_mode]; status = "FORWARD"; name = "RIDE MODE";
        note = "Forward drive  /  sample telemetry";
        demo_speed = 24 + active_mode * 12;
    } else if (state == PREVIEW_REVERSE) {
        gear = "R"; color = ORANGE; status = "REVERSE ACTIVE"; name = "REVERSE";
        note = "RX held  /  reverse confirmed (preview)"; demo_speed = 3;
    } else if (state == PREVIEW_STALE || state == PREVIEW_FAULT) {
        gear = "-"; color = state == PREVIEW_FAULT ? RED : MUTED;
        status = state == PREVIEW_FAULT ? "ESC FAULT" : "NO CAN DATA";
        name = state == PREVIEW_FAULT ? "CHECK CONTROLLER" : "UNCONFIRMED";
        note = "Drive state unavailable  /  no commands sent";
    }
    lv_label_set_text(gear_text, gear); lv_obj_center(gear_text);
    lv_obj_set_style_text_color(gear_text, lv_color_hex(color), 0);
    lv_obj_set_style_border_color(gear_ring, lv_color_hex(color), 0);
    lv_label_set_text(gear_name, name); lv_obj_set_style_text_color(gear_name, lv_color_hex(color), 0);
    lv_label_set_text(drive_status, status); lv_obj_set_style_text_color(drive_status, lv_color_hex(color), 0);
    lv_label_set_text(dash_note, note);
    lv_label_set_text(ecu_temp, state >= PREVIEW_STALE ? "--" : "38");
    lv_label_set_text(motor_temp, state >= PREVIEW_STALE ? "--" : "44");
    lv_label_set_text(can_status, state == PREVIEW_STALE ? "CAN --" : "CAN");
    lv_obj_set_style_text_color(can_status, lv_color_hex(state == PREVIEW_STALE ? MUTED : state == PREVIEW_FAULT ? RED : GREEN), 0);
    if (state == PREVIEW_STALE || state == PREVIEW_FAULT) {
        lv_label_set_text(speed_label, "--"); lv_label_set_text(motor_current, "--");
    } else {
        lv_label_set_text_fmt(speed_label, "%02d", demo_speed);
        lv_label_set_text(motor_current, state == PREVIEW_PARK ? "0.0" : state == PREVIEW_REVERSE ? "-2.1" : "18.4");
    }
    refresh_modes();
}

static void tick(lv_timer_t *timer) {
    (void)timer; tick_count++;
#ifdef UI_7B_HARDWARE
    if (tick_count % 3 == 0) hardware_update();
#endif
    if (toast_until && (int32_t)(lv_tick_get() - toast_until) >= 0) {
        lv_obj_add_flag(toast_box, LV_OBJ_FLAG_HIDDEN); toast_until = 0;
    }
    if (animate_demo && scenario >= PREVIEW_DRIVE_1 && scenario <= PREVIEW_DRIVE_3) {
        int phase = (tick_count / 4) % 20;
        lv_label_set_text_fmt(speed_label, "%02d", demo_speed + (phase < 10 ? phase : 20 - phase) - 5);
    }
}

void preview_ui_init(void) {
#ifdef UI_7B_HARDWARE
    brightness=backend_brightness(); target_id=backend_target();
#endif
    root = box(NULL, 0, 0, 1024, 600, BLACK, 0, 0);
    for (int i = 0; i < 4; ++i) pages[i] = box(root, 0, 88, 1024, 447, BLACK, 0, 0);
    dashboard(); modes_page(); bms_page(); settings_page(); top_chrome();
    toast_box = box(root, 110, 482, 804, 46, 0x1f282e, ORANGE, 8);
    toast_text = text(toast_box, "", 12, 13, 780, F16, WHITE, CENTER);
    lv_obj_add_flag(toast_box, LV_OBJ_FLAG_HIDDEN);
    lv_scr_load(root); preview_ui_page(PREVIEW_DASH); preview_ui_scenario(PREVIEW_PARK);
#ifdef UI_7B_HARDWARE
    hardware_update();
#endif
    lv_timer_create(tick, 100, NULL);
}

#ifdef UI_7B_HARDWARE
static void number(lv_obj_t *o, const char *fmt, float value, bool valid) {
    char s[32]; if(valid) snprintf(s,sizeof(s),fmt,(double)value); else strcpy(s,"--");
    lv_label_set_text(o,s);
}
static void hardware_update(void) {
    telemetry_t t; backend_snapshot(&t);
    vesc_ride_safety_t safety={0};
    bool safe=vesc_ride_mode_get_safety(&safety) && safety.valid && t.fresh;
    /* SELECT has no correlated config acknowledgement in this Lisp version.
     * Observe a newer safety snapshot instead of inventing a Save response. */
    if(pending_mode>=0 && safe && safety.epoch!=pending_mode_epoch && safety.current_profile==pending_mode) {
        toast("Requested mode observed on ESC.");pending_mode=-1;
    } else if(pending_mode>=0 && lv_tick_get()-pending_mode_at>4500) {toast("Requested mode not confirmed.");pending_mode=-1;}
    if(pending_park && safety.command_seq==pending_park && !safety.command_pending) {
        char msg[96]; snprintf(msg,sizeof(msg),"Park / Drive: %s (result %u)",safety.command_result==0?"ESC confirmed":"ESC refused",safety.command_result);
        toast(msg);pending_park=0;
    } else if(pending_park && lv_tick_get()-pending_park_at>1500) {toast("Park / Drive timed out. State not confirmed.");pending_park=0;}
    preview_scenario_t next=PREVIEW_STALE;
    if(t.fresh && t.fault) next=PREVIEW_FAULT;
    else if(safe) {
        if(safety.state==VESC_RIDE_SAFETY_PARK) next=PREVIEW_PARK;
        else if(safety.state==VESC_RIDE_SAFETY_REVERSE_ACTIVE || safety.state==VESC_RIDE_SAFETY_REVERSE_READY) next=PREVIEW_REVERSE;
        else if(safety.state==VESC_RIDE_SAFETY_FORWARD && safety.current_profile<3)
            next=(preview_scenario_t)(PREVIEW_DRIVE_1+safety.current_profile);
        else if(safety.state==VESC_RIDE_SAFETY_FAULT) next=PREVIEW_FAULT;
    }
    if(scenario!=next) preview_ui_scenario(next);
    if(next==PREVIEW_REVERSE) lv_label_set_text(drive_status,safety.state==VESC_RIDE_SAFETY_REVERSE_READY?"REVERSE READY":"REVERSE ACTIVE");
    lv_label_set_text(can_status,t.fresh ? "CAN" : "CAN --");
    lv_obj_set_style_text_color(can_status,lv_color_hex(t.fresh ? GREEN:MUTED),0);
    lv_label_set_text(dash_note, safe ? "Hold gear ring to request Park / Drive" : "Waiting for fresh ESC / Lisp state");
    number(speed_label,"%02.0f",t.speed<0?-t.speed:t.speed,t.fresh);
    number(motor_current,"%.1f",t.current,t.fresh);
    number(ecu_temp,"%.0f",t.ecu_temp,t.fresh);
    number(motor_temp,"%.0f",t.motor_temp,t.fresh);
    number(odo_label,"%.0f",t.odo,t.fresh);
    number(trip_label,"%.1f",t.trip,t.fresh);
    number(soc_label,"%.0f",t.soc,t.fresh);
    number(voltage_label,"%.1f",t.voltage,t.fresh);
    lv_label_set_text(range_label,"--"); lv_label_set_text(battery_temp,"--");
    for(int i=0;i<20;++i) lv_obj_set_style_bg_color(bars[i],lv_color_hex(t.fresh && i*5<t.soc?GREEN:LINE),0);
    vesc_ride_config_t cfg={0};
    config_loaded=vesc_ride_mode_get_config(&cfg) && cfg.valid && safe;
    if(pending_config && cfg.response_seq==pending_config) {
        char msg[96];snprintf(msg,sizeof(msg),"Ride settings: %s (result %u)",cfg.last_result==0?"ESC confirmed":"ESC refused",cfg.last_result);
        toast(msg);pending_config=0;
    } else if(pending_config && lv_tick_get()-pending_config_at>4500) {toast("Ride request timed out. No confirmation.");pending_config=0;}
    if(config_loaded) {
        if(config_epoch!=cfg.epoch && !pending_config) {
            config_epoch=cfg.epoch;
            for(int i=0;i<3;++i) draft_amps[i]=cfg.mode_current_dA[i]/10;
            reverse_enabled=cfg.reverse_enabled;
        }
        set_button_text(reverse_button,reverse_enabled?"Enabled":"Disabled");
        char s[100]; snprintf(s,sizeof(s),"Motor current limit / ESC maximum: %.1f A",cfg.esc_current_max_dA/10.0);
        lv_label_set_text(mode_heading,s);
        snprintf(s,sizeof(s),"%.1f km/h  /  %.1f A",cfg.reverse_speed_dkmh/10.0,cfg.reverse_current_dA/10.0);
        lv_label_set_text(reverse_limits,s);
        for(int i=0;i<3;++i) {
            lv_label_set_text_fmt(mode_values[i],"%d",draft_amps[i]);
            float limit=draft_amps[i]<cfg.esc_current_max_dA/10.0f?draft_amps[i]:cfg.esc_current_max_dA/10.0f;
            snprintf(s,sizeof(s),"Effective %.1f A",limit); lv_label_set_text(effective_values[i],s);
        }
    } else {
        set_button_text(reverse_button,"Unknown");
        lv_label_set_text(mode_heading,"Waiting for Lisp configuration");
        lv_label_set_text(reverse_limits,"-- km/h  /  -- A");
        for(int i=0;i<3;++i) {lv_label_set_text(mode_values[i],"--");lv_label_set_text(effective_values[i],"Not confirmed");}
    }
    bms_snapshot_t b={0};
    bool live=bms_model_get(&b) && bms_model_link_state()==BMS_LINK_LIVE;
    lv_obj_set_style_text_color(ble_status,lv_color_hex(live?BLUE:MUTED),0);
    if(live) {
        if(b.valid_mask&BMS_V_SOC) number(soc_label,"%.0f",b.soc_permille/10.0f,true);
        if(b.valid_mask&BMS_V_PACK_MV) number(voltage_label,"%.1f",b.pack_mv/1000.0f,true);
        if(b.temp_valid_mask&1) number(battery_temp,"%.0f",b.temp_deci_c[0]/10.0f,true);
        for(int i=0;i<20;++i) lv_obj_set_style_bg_color(bars[i],lv_color_hex((b.valid_mask&BMS_V_SOC)&&i*50<b.soc_permille?GREEN:LINE),0);
    }
    if(current_page==PREVIEW_BMS && tick_count%9==0 && !lv_obj_is_scrolling(bms_body)) {
        int scroll=lv_obj_get_scroll_y(bms_body);
        lv_obj_clean(bms_body); hardware_bms_view();
        lv_obj_scroll_to_y(bms_body,scroll,LV_ANIM_OFF);
    }
}
static void hardware_bms_view(void) {
    bms_snapshot_t b={0};
    bool live=bms_model_get(&b) && bms_model_link_state()==BMS_LINK_LIVE;
    if(bms_tab==0) {
        const char *names[]={"STATE OF CHARGE","PACK VOLTAGE","DISCHARGE CURRENT","REMAINING","CELL DELTA","MOS TEMPERATURE"};
        const char *units[]={"%","V","A","Ah","mV","°C"};
        float values[]={b.soc_permille/10.f,b.pack_mv/1000.f,b.pack_current_ma/1000.f,b.remaining_mah/1000.f,b.cell_delta_mv,b.mos_temp_deci_c/10.f};
        uint32_t masks[]={BMS_V_SOC,BMS_V_PACK_MV,BMS_V_CURRENT,BMS_V_REMAINING,BMS_V_CELLS,BMS_V_MOS_TEMP};
        for(int i=0;i<6;++i) {
            char s[24]; if(live && (b.valid_mask&masks[i])) snprintf(s,sizeof(s),"%.1f",values[i]); else strcpy(s,"--");
            stat(bms_body,names[i],s,units[i],32+(i%3)*328,10+(i/3)*154,304,i==0?GREEN:WHITE);
        }
    } else {
        // Scroll accommodates both 24S and 32S JK layouts without hiding cells.
        int count=live ? b.cell_count : 16;
        lv_obj_add_flag(bms_body,LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(bms_body,LV_DIR_VER);
        for(int i=0;i<count && i<BMS_MAX_CELLS;++i) {
            lv_obj_t *cell=box(bms_body,32+(i%8)*122,14+(i/8)*143,110,126,PANEL,LINE,9);
            char title[24],value[24]; snprintf(title,sizeof(title),"%s %02d",bms_tab==1?"CELL":"WIRE",i+1);
            text(cell,title,9,13,92,&lv_font_montserrat_12,MUTED,CENTER);
            uint32_t valid=bms_tab==1?b.cell_valid_mask:b.wire_res_valid_mask;
            if(live && (valid&(1u<<i))) snprintf(value,sizeof(value),bms_tab==1?"%.3f":"%.1f",bms_tab==1?b.cell_mv[i]/1000.0:(double)b.wire_res_mohm[i]);
            else strcpy(value,"--");
            text(cell,value,7,40,96,A32,WHITE,CENTER);
            text(cell,bms_tab==1?"V":"mOhm",7,83,96,&lv_font_montserrat_12,MUTED,CENTER);
        }
    }
    if(bms_tab==0) {
        lv_obj_clear_flag(bms_body,LV_OBJ_FLAG_SCROLLABLE);
        text(bms_body,bms_model_link_state_str(bms_model_link_state()),34,325,900,F16,MUTED,LEFT);
    }
}
#endif

static lv_obj_t *welcome, *welcome_title, *welcome_line;
static uint32_t welcome_started;
static void welcome_tick(lv_timer_t *timer) {
    uint32_t elapsed = lv_tick_elaps(welcome_started);
    if(elapsed >= 2200) {
        lv_scr_load(root);
        lv_obj_del(welcome); welcome = NULL;
        lv_timer_del(timer);
        return;
    }
    unsigned fade = elapsed < 700 ? elapsed * 255 / 700 : 255;
    if(elapsed > 1800) fade = (2200 - elapsed) * 255 / 400;
    lv_obj_set_style_text_opa(welcome_title, fade, 0);
    unsigned width = elapsed < 1200 ? 2 + elapsed * 478 / 1200 : 480;
    lv_obj_set_width(welcome_line, width);
    lv_obj_set_x(welcome_line, (1024 - width) / 2);
    lv_obj_set_style_bg_opa(welcome_line, fade, 0);
}
void preview_ui_startup(void) {
    if(welcome) return;
    welcome = lv_obj_create(NULL);
    lv_obj_remove_style_all(welcome);
    lv_obj_set_size(welcome, 1024, 600);
    lv_obj_set_style_bg_color(welcome, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(welcome, LV_OPA_COVER, 0);
    welcome_title = text(welcome, DISPLAY_BRAND_NAME, 62, 232, 900, &lv_font_montserrat_48, WHITE, CENTER);
    lv_obj_set_style_text_opa(welcome_title, 0, 0);
    text(welcome, "WELCOME", 62, 174, 900, F16, ORANGE, CENTER);
    text(welcome, "READY FOR THE RIDE", 62, 374, 900, F16, MUTED, CENTER);
    welcome_line = box(welcome, 511, 330, 2, 4, ORANGE, ORANGE, 2);
    welcome_started = lv_tick_get();
    lv_scr_load(welcome);
    lv_timer_create(welcome_tick, 33, NULL);
}

/* Exercise state/display and real button callbacks without device I/O. */
int preview_ui_self_test(void) {
    int errors = 0;
    const char *gears[] = {"P", "1", "2", "3", "R", "-", "-"};
    for (int i = 0; i < 7; ++i) {
        preview_ui_scenario((preview_scenario_t)i);
        if (strcmp(lv_label_get_text(gear_text), gears[i])) errors++;
        if (i >= PREVIEW_STALE && strcmp(lv_label_get_text(speed_label), "--")) errors++;
    }
    for (int i = 0; i < 4; ++i) {
        lv_event_send(nav[i], LV_EVENT_CLICKED, NULL);
        if (current_page != i || lv_obj_has_flag(pages[i], LV_OBJ_FLAG_HIDDEN)) errors++;
    }
    preview_ui_scenario(PREVIEW_REVERSE);
    int before = active_mode;
    lv_event_send(mode_selected[(before + 1) % 3], LV_EVENT_CLICKED, NULL);
    if (active_mode != before) errors++;
    preview_ui_scenario(PREVIEW_PARK);
    lv_event_send(mode_selected[0], LV_EVENT_CLICKED, NULL);
    if (active_mode != 0 || effective(100) != 70) errors++;
    for (int i = 0; i < 3; ++i) {
        lv_event_send(bms_tabs[i], LV_EVENT_CLICKED, NULL);
        if (bms_tab != i) errors++;
    }
    lv_obj_add_flag(toast_box, LV_OBJ_FLAG_HIDDEN); toast_until = 0;
    return errors;
}
