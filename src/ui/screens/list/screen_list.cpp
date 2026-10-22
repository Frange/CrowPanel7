
#include <Arduino.h>
#include <WiFi.h>

#include "screen_list.h"
#include "screen_list_internal.h"
#include "screen_list_ui.h"
#include "screen_list_data.h"

#include "../../../services/market_service.h"

ScreenListState screen_list_state;

namespace {

void list_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *screen =
        static_cast<lv_obj_t *>(timer->user_data);

    if (lv_scr_act() != screen) {
        if (timer == screen_list_state.timer) {
            screen_list_state.timer = nullptr;
        }

        lv_timer_del(timer);
        return;
    }

    screen_list_update_wifi_indicator();
    screen_list_update_rows();

    uint32_t now = millis();

    if (!screen_list_state.first_refresh_requested ||
        static_cast<uint32_t>(
            now - screen_list_state.last_refresh_request
        ) >= AUTO_REFRESH_MS) {

        if (market_service_request_refresh()) {
            screen_list_state.first_refresh_requested = true;
            screen_list_state.last_refresh_request = now;
        }
    }
}

void reset_state()
{
    auto &state = screen_list_state;

    state.screen = nullptr;
    state.wifi_indicator = nullptr;
    state.refresh_status = nullptr;
    state.list_content = nullptr;
    state.empty_label = nullptr;

    for (size_t i = 0; i < MAX_ROWS; ++i) {
        state.ticker_labels[i] = nullptr;
        state.price_labels[i] = nullptr;
        state.open_labels[i] = nullptr;
        state.close_labels[i] = nullptr;
        state.change_labels[i] = nullptr;
        state.delete_buttons[i] = nullptr;
    }

    state.timer = nullptr;
    state.last_refresh_request = millis();
    state.first_refresh_requested = false;
}

} // namespace

lv_obj_t *screen_list_create()
{
    auto &state = screen_list_state;

    if (state.timer != nullptr) {
        lv_timer_del(state.timer);
        state.timer = nullptr;
    }

    reset_state();

    lv_obj_t *scr = lv_obj_create(nullptr);
    state.screen = scr;

    screen_list_build_ui();

    if (market_service_request_refresh()) {
        state.first_refresh_requested = true;
        state.last_refresh_request = millis();
    }

    state.timer = lv_timer_create(
        list_timer_cb,
        UI_POLL_MS,
        scr
    );

    return scr;
}
