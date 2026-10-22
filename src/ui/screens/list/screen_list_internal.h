#pragma once

#include <Arduino.h>
#include <lvgl.h>

constexpr size_t MAX_ROWS = 40;
constexpr uint32_t AUTO_REFRESH_MS = 300000;
constexpr uint32_t UI_POLL_MS = 1000;

struct ScreenListState {
    lv_obj_t *screen = nullptr;
    lv_obj_t *wifi_indicator = nullptr;
    lv_obj_t *refresh_status = nullptr;
    lv_obj_t *list_content = nullptr;
    lv_obj_t *empty_label = nullptr;

    lv_obj_t *ticker_labels[MAX_ROWS] = {};
    lv_obj_t *price_labels[MAX_ROWS] = {};
    lv_obj_t *open_labels[MAX_ROWS] = {};
    lv_obj_t *close_labels[MAX_ROWS] = {};
    lv_obj_t *change_labels[MAX_ROWS] = {};
    lv_obj_t *delete_buttons[MAX_ROWS] = {};

    lv_timer_t *timer = nullptr;

    uint32_t last_refresh_request = 0;
    bool first_refresh_requested = false;
    bool wifi_was_connected = false;

};

extern ScreenListState screen_list_state;

void screen_list_update_rows();
void screen_list_update_wifi_indicator();
void screen_list_refresh_status();
void screen_list_build_ui();

void screen_list_ticker_row_event_cb(lv_event_t *e);
void screen_list_refresh_btn_event_cb(lv_event_t *e);
void screen_list_add_btn_event_cb(lv_event_t *e);
void screen_list_settings_btn_event_cb(lv_event_t *e);
void screen_list_delete_btn_event_cb(lv_event_t *e);