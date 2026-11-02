#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

#include "screen_list.h"
#include "screen_list_internal.h"

#include "../../../services/market_service.h"

ScreenListState screen_list_state;

void screen_list_set_text(lv_obj_t *label, const char *text)
{
    if (label == nullptr || text == nullptr) {
        return;
    }

    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

namespace {

void list_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    auto &state = screen_list_state;

    screen_list_update_wifi_indicator();

    uint32_t version = market_service_data_version();
    size_t count = storage_ticker_count();

    if (version != state.painted_version ||
        count != state.painted_count) {
        state.painted_version = version;
        state.painted_count = count;
        screen_list_update_rows();
    }

    screen_list_refresh_status();
}

void screen_delete_cb(lv_event_t *e)
{
    auto &state = screen_list_state;

    // Solo limpiar si la pantalla que se borra es la que esta registrada.
    if (e != nullptr && lv_event_get_target(e) != state.screen) {
        return;
    }

    if (state.timer != nullptr) {
        lv_timer_del(state.timer);
        state.timer = nullptr;
    }

    state.screen = nullptr;
    state.wifi_indicator = nullptr;
    state.refresh_status = nullptr;
    state.list_content = nullptr;
    state.empty_label = nullptr;

    for (size_t i = 0; i < MAX_ROWS; ++i) {
        state.rows[i] = nullptr;
        state.ticker_labels[i] = nullptr;
        state.price_labels[i] = nullptr;
        state.open_labels[i] = nullptr;
        state.close_labels[i] = nullptr;
        state.change_labels[i] = nullptr;
        state.delete_buttons[i] = nullptr;
    }
}

} // namespace

lv_obj_t *screen_list_create()
{
    auto &state = screen_list_state;

    // Si quedara una pantalla anterior viva, limpiar su estado.
    screen_delete_cb(nullptr);

    state.painted_version = 0xFFFFFFFFu;
    state.painted_count = 0;
    state.painted_wifi = -1;

    lv_obj_t *scr = lv_obj_create(nullptr);
    state.screen = scr;

    lv_obj_add_event_cb(scr, screen_delete_cb, LV_EVENT_DELETE, nullptr);

    screen_list_build_ui();

    state.painted_version = market_service_data_version();
    state.painted_count = storage_ticker_count();

    state.timer = lv_timer_create(list_timer_cb, UI_POLL_MS, nullptr);

    return scr;
}
