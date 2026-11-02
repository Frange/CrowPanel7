#include "ui_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "screens/list/screen_list.h"
#include "screens/detail/screen_detail.h"
#include "screens/add/screen_add.h"
#include "screens/wifi/screen_wifi.h"
#include "screens/settings/screen_settings.h"

#include "../display_config.h"
#include "../services/settings_service.h"
#include "../services/market_service.h"

namespace {

constexpr uint32_t GLOBAL_TICK_MS = 1000;
constexpr uint8_t  DIMMED_BRIGHTNESS = 8;

lv_obj_t *current_screen = nullptr;

bool screen_dimmed = false;
bool wifi_was_connected = false;
uint32_t last_refresh_request = 0;
bool first_refresh_done = false;

// Atenuacion por inactividad: baja el brillo tras N minutos sin tocar
// la pantalla y lo recupera con el primer toque.
void handle_idle_dimming()
{
    const AppSettings &cfg = settings_get();

    if (cfg.dim_minutes == 0) {
        if (screen_dimmed) {
            ui_apply_brightness();
        }
        return;
    }

    uint32_t idle_ms = lv_disp_get_inactive_time(nullptr);
    uint32_t limit_ms = static_cast<uint32_t>(cfg.dim_minutes) * 60000u;

    if (!screen_dimmed && idle_ms >= limit_ms) {
        screen_dimmed = true;
        display_set_brightness(DIMMED_BRIGHTNESS);
    } else if (screen_dimmed && idle_ms < GLOBAL_TICK_MS) {
        ui_apply_brightness();
    }
}

// Actualizacion automatica de cotizaciones, independiente de la
// pantalla que este abierta.
void handle_auto_refresh()
{
    bool connected = WiFi.status() == WL_CONNECTED;

    if (!connected) {
        wifi_was_connected = false;
        return;
    }

    uint32_t now = millis();
    uint32_t interval_ms =
        static_cast<uint32_t>(settings_get().refresh_minutes) * 60000u;

    bool just_connected = !wifi_was_connected;
    wifi_was_connected = true;

    bool due =
        just_connected ||
        !first_refresh_done ||
        static_cast<uint32_t>(now - last_refresh_request) >= interval_ms;

    if (due && market_service_request_refresh()) {
        first_refresh_done = true;
        last_refresh_request = now;
    }
}

void global_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    handle_idle_dimming();
    handle_auto_refresh();
}

} // namespace

void ui_init()
{
    current_screen = screen_list_create();

    if (current_screen != nullptr) {
        lv_scr_load(current_screen);
    }

    lv_timer_create(global_timer_cb, GLOBAL_TICK_MS, nullptr);
}

void ui_apply_brightness()
{
    screen_dimmed = false;
    display_set_brightness(settings_get().brightness);
}

void ui_change_screen(ScreenState state)
{
    lv_obj_t *new_screen = nullptr;

    switch (state) {
        case SCREEN_LIST:
            new_screen = screen_list_create();
            break;

        case SCREEN_DETAIL:
            new_screen = screen_detail_create();
            break;

        case SCREEN_ADD:
            new_screen = screen_add_create();
            break;

        case SCREEN_WIFI:
            new_screen = screen_wifi_create();
            break;

        case SCREEN_SETTINGS:
            new_screen = screen_settings_create();
            break;

        default:
            return;
    }

    if (new_screen == nullptr) {
        return;
    }

    lv_obj_t *old_screen = current_screen;

    lv_scr_load(new_screen);
    current_screen = new_screen;

    if (old_screen != nullptr && old_screen != new_screen) {
        // Al borrarse, cada pantalla recibe LV_EVENT_DELETE y limpia
        // sus timers y punteros.
        lv_obj_del_async(old_screen);
    }
}
