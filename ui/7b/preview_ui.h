#pragma once
/* Portable LVGL view prototype; mock state is not an ESC controller. */
typedef enum { PREVIEW_DASH, PREVIEW_MODES, PREVIEW_BMS, PREVIEW_SETTINGS } preview_page_t;
typedef enum {
    PREVIEW_PARK, PREVIEW_DRIVE_1, PREVIEW_DRIVE_2, PREVIEW_DRIVE_3,
    PREVIEW_REVERSE, PREVIEW_STALE, PREVIEW_FAULT
} preview_scenario_t;
void preview_ui_init(void);
void preview_ui_startup(void);
void preview_ui_page(preview_page_t page);
void preview_ui_scenario(preview_scenario_t scenario);
void preview_ui_bms_tab(int tab);
int preview_ui_self_test(void);
