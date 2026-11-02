#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "../../../services/storage_service.h"

constexpr size_t MAX_ROWS = STORAGE_MAX_TICKERS;
constexpr uint32_t UI_POLL_MS = 1000;

struct ScreenListState {
    lv_obj_t *screen = nullptr;
    lv_obj_t *wifi_indicator = nullptr;
    lv_obj_t *refresh_status = nullptr;
    lv_obj_t *list_content = nullptr;
    lv_obj_t *empty_label = nullptr;

    lv_obj_t *rows[MAX_ROWS] = {};
    lv_obj_t *ticker_labels[MAX_ROWS] = {};
    lv_obj_t *price_labels[MAX_ROWS] = {};
    lv_obj_t *open_labels[MAX_ROWS] = {};
    lv_obj_t *close_labels[MAX_ROWS] = {};
    lv_obj_t *change_labels[MAX_ROWS] = {};
    lv_obj_t *delete_buttons[MAX_ROWS] = {};

    lv_timer_t *timer = nullptr;

    // Ultimo estado pintado, para repintar solo cuando cambia algo.
    uint32_t painted_version = 0xFFFFFFFFu;
    size_t painted_count = 0;
    int painted_wifi = -1;
};

extern ScreenListState screen_list_state;

// Cambia el texto de una etiqueta solo si es distinto (evita invalidar
// y repintar la fila cuando no hay cambios).
void screen_list_set_text(lv_obj_t *label, const char *text);

// screen_list_data.cpp
void screen_list_update_rows();
void screen_list_update_wifi_indicator();
void screen_list_refresh_status();

// screen_list_ui.cpp
void screen_list_build_ui();

// screen_list_events.cpp
void screen_list_ticker_row_event_cb(lv_event_t *e);
void screen_list_refresh_btn_event_cb(lv_event_t *e);
void screen_list_add_btn_event_cb(lv_event_t *e);
void screen_list_settings_btn_event_cb(lv_event_t *e);
void screen_list_delete_btn_event_cb(lv_event_t *e);
