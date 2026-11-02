#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <esp_heap_caps.h>

#include "screen_settings.h"
#include "../../ui_manager.h"
#include "../../ui_dialog.h"
#include "../../../display_config.h"
#include "../../../services/settings_service.h"
#include "../../../services/storage_service.h"
#include "../../../services/market_service.h"

namespace {

constexpr uint32_t INFO_REFRESH_MS = 2000;

const uint16_t refresh_options[] = {1, 2, 5, 15, 30, 60};
const char *refresh_options_text =
    "1 min\n2 min\n5 min\n15 min\n30 min\n60 min";

const uint16_t dim_options[] = {0, 1, 5, 15, 30};
const char *dim_options_text =
    "Nunca\n1 min\n5 min\n15 min\n30 min";

lv_obj_t *brightness_value = nullptr;
lv_obj_t *info_label = nullptr;
lv_obj_t *action_status = nullptr;
lv_timer_t *info_timer = nullptr;

template <size_t N>
uint16_t index_of(const uint16_t (&options)[N], uint16_t value)
{
    uint16_t best = 0;

    for (size_t i = 0; i < N; ++i) {
        if (options[i] == value) {
            return static_cast<uint16_t>(i);
        }
        if (options[i] < value) {
            best = static_cast<uint16_t>(i);
        }
    }

    return best;
}

void set_action_status(const char *text, uint32_t color)
{
    if (action_status != nullptr) {
        lv_label_set_text(action_status, text);
        lv_obj_set_style_text_color(
            action_status, lv_color_hex(color), 0
        );
    }
}

// --------------------------------------------------
// Panel de informacion del sistema
// --------------------------------------------------

void update_info()
{
    if (info_label == nullptr) {
        return;
    }

    char wifi_line[96];

    if (WiFi.status() == WL_CONNECTED) {
        snprintf(
            wifi_line, sizeof(wifi_line),
            "Red: %s\nIP: %s   Senal: %d dBm",
            WiFi.SSID().c_str(),
            WiFi.localIP().toString().c_str(),
            WiFi.RSSI()
        );
    } else {
        String saved_ssid;
        String saved_pass;

        if (storage_get_wifi_credentials(saved_ssid, saved_pass)) {
            snprintf(
                wifi_line, sizeof(wifi_line),
                "Red: %s (desconectado)\nIP: --",
                saved_ssid.c_str()
            );
        } else {
            snprintf(
                wifi_line, sizeof(wifi_line),
                "Red: sin configurar\nIP: --"
            );
        }
    }

    uint32_t uptime_s = millis() / 1000;
    uint32_t hours = uptime_s / 3600;
    uint32_t minutes = (uptime_s / 60) % 60;

    uint32_t last_refresh = market_service_last_refresh_ms();
    char refresh_line[64];

    if (last_refresh == 0) {
        snprintf(refresh_line, sizeof(refresh_line),
                 "Ultima actualizacion: --");
    } else {
        uint32_t ago_s = (millis() - last_refresh) / 1000;

        if (ago_s < 60) {
            snprintf(refresh_line, sizeof(refresh_line),
                     "Ultima actualizacion: hace %lu s",
                     (unsigned long)ago_s);
        } else {
            snprintf(refresh_line, sizeof(refresh_line),
                     "Ultima actualizacion: hace %lu min",
                     (unsigned long)(ago_s / 60));
        }
    }

    lv_label_set_text_fmt(
        info_label,
        "%s\n\n"
        "Valores guardados: %u de %u\n"
        "%s\n\n"
        "RAM libre: %u KB   PSRAM libre: %u KB\n"
        "Encendido: %luh %02lumin\n"
        "Chip: %s rev%d  CPU: %u MHz",
        wifi_line,
        (unsigned int)storage_ticker_count(),
        (unsigned int)STORAGE_MAX_TICKERS,
        refresh_line,
        (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
        (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
        (unsigned long)hours,
        (unsigned long)minutes,
        ESP.getChipModel(),
        ESP.getChipRevision(),
        (unsigned int)ESP.getCpuFreqMHz()
    );
}

void info_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    update_info();
}

// --------------------------------------------------
// Eventos
// --------------------------------------------------

void back_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_LIST);
}

void screen_delete_cb(lv_event_t *e)
{
    (void)e;

    if (info_timer != nullptr) {
        lv_timer_del(info_timer);
        info_timer = nullptr;
    }

    brightness_value = nullptr;
    info_label = nullptr;
    action_status = nullptr;
}

void brightness_changed_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    // Brillo inmediato mientras se arrastra...
    ui_apply_brightness();
    display_set_brightness(static_cast<uint8_t>(value));

    if (brightness_value != nullptr) {
        lv_label_set_text_fmt(
            brightness_value, "%d%%",
            (int)((value * 100) / 255)
        );
    }
}

void brightness_released_cb(lv_event_t *e)
{
    // ...y se guarda en NVS solo al soltar.
    lv_obj_t *slider = lv_event_get_target(e);
    settings_set_brightness(
        static_cast<uint8_t>(lv_slider_get_value(slider))
    );
}

void refresh_changed_cb(lv_event_t *e)
{
    lv_obj_t *dropdown = lv_event_get_target(e);
    uint16_t index = lv_dropdown_get_selected(dropdown);

    if (index < sizeof(refresh_options) / sizeof(refresh_options[0])) {
        settings_set_refresh_minutes(refresh_options[index]);
    }
}

void dim_changed_cb(lv_event_t *e)
{
    lv_obj_t *dropdown = lv_event_get_target(e);
    uint16_t index = lv_dropdown_get_selected(dropdown);

    if (index < sizeof(dim_options) / sizeof(dim_options[0])) {
        settings_set_dim_minutes(dim_options[index]);
    }
}

void forget_wifi_confirmed(void *user_data)
{
    (void)user_data;
    storage_forget_wifi();
    set_action_status("Red WiFi olvidada.", 0xFFCC66);
    update_info();
}

void forget_wifi_cb(lv_event_t *e)
{
    (void)e;
    ui_confirm(
        "Olvidar red WiFi",
        "Se borraran las credenciales guardadas\n"
        "y se cerrara la conexion actual.",
        "Olvidar",
        true,
        forget_wifi_confirmed,
        nullptr
    );
}

void clear_tickers_confirmed(void *user_data)
{
    (void)user_data;

    if (storage_clear_tickers()) {
        market_service_clear_cache();
        set_action_status("Lista de valores borrada.", 0xFFCC66);
    } else {
        set_action_status("No se pudo borrar la lista.", 0xFF5555);
    }

    update_info();
}

void clear_tickers_cb(lv_event_t *e)
{
    (void)e;
    ui_confirm(
        "Borrar todos los valores",
        "Se eliminaran todas las acciones y ETFs\n"
        "guardados. Esta accion no se puede deshacer.",
        "Borrar todo",
        true,
        clear_tickers_confirmed,
        nullptr
    );
}

void restart_confirmed(void *user_data)
{
    (void)user_data;
    Serial.println("[SETTINGS] Reiniciando...");
    delay(100);
    ESP.restart();
}

void restart_cb(lv_event_t *e)
{
    (void)e;
    ui_confirm(
        "Reiniciar",
        "Se reiniciara el dispositivo.",
        "Reiniciar",
        false,
        restart_confirmed,
        nullptr
    );
}

// --------------------------------------------------
// Construccion de widgets
// --------------------------------------------------

lv_obj_t *make_card(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *card = lv_obj_create(parent);

    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    return card;
}

lv_obj_t *make_label(
    lv_obj_t *parent,
    const char *text,
    int x,
    int y,
    uint32_t color,
    const lv_font_t *font = &lv_font_montserrat_14)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t *make_dropdown(
    lv_obj_t *parent,
    const char *options,
    uint16_t selected,
    int x,
    int y,
    lv_event_cb_t cb)
{
    lv_obj_t *dropdown = lv_dropdown_create(parent);

    lv_dropdown_set_options_static(dropdown, options);
    lv_dropdown_set_selected(dropdown, selected);
    lv_obj_set_size(dropdown, 180, 42);
    lv_obj_set_pos(dropdown, x, y);
    lv_obj_set_style_bg_color(dropdown, lv_color_hex(0x303030), 0);

    lv_obj_add_event_cb(dropdown, cb, LV_EVENT_VALUE_CHANGED, nullptr);

    return dropdown;
}

lv_obj_t *make_button(
    lv_obj_t *parent,
    const char *text,
    int x,
    int y,
    int w,
    uint32_t color,
    lv_event_cb_t cb)
{
    lv_obj_t *button = lv_btn_create(parent);

    lv_obj_set_size(button, w, 42);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return button;
}

} // namespace

lv_obj_t *screen_settings_create()
{
    const AppSettings &cfg = settings_get();

    lv_obj_t *screen = lv_obj_create(nullptr);

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x121212), 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen, screen_delete_cb, LV_EVENT_DELETE, nullptr);

    // Barra superior
    lv_obj_t *top_bar = lv_obj_create(screen);

    lv_obj_set_size(top_bar, 800, 65);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1F1F1F), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_button = lv_btn_create(top_bar);

    lv_obj_set_size(back_button, 100, 42);
    lv_obj_align(back_button, LV_ALIGN_LEFT_MID, 15, 0);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back_button, 8, 0);
    lv_obj_add_event_cb(back_button, back_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Volver");
    lv_obj_center(back_label);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Ajustes");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // --------------------------------------------------
    // Tarjeta izquierda: pantalla y datos
    // --------------------------------------------------
    lv_obj_t *left = make_card(screen, 20, 80, 370, 385);

    make_label(left, "Pantalla y datos", 0, 0, 0x007ACC,
               &lv_font_montserrat_16);

    // Brillo
    make_label(left, "Brillo de pantalla", 0, 40, 0xFFFFFF);

    brightness_value = make_label(left, "", 290, 40, 0xAAAAAA);
    lv_obj_set_width(brightness_value, 48);
    lv_obj_set_style_text_align(brightness_value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text_fmt(
        brightness_value, "%d%%", (int)((cfg.brightness * 100) / 255)
    );

    lv_obj_t *slider = lv_slider_create(left);
    lv_obj_set_size(slider, 320, 16);
    lv_obj_set_pos(slider, 8, 74);
    lv_slider_set_range(slider, 10, 255);
    lv_slider_set_value(slider, cfg.brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x007ACC), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x007ACC), LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 6, LV_PART_KNOB);

    lv_obj_add_event_cb(slider, brightness_changed_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(slider, brightness_released_cb, LV_EVENT_RELEASED, nullptr);

    // Intervalo de actualizacion
    make_label(left, "Actualizar cotizaciones cada", 0, 126, 0xFFFFFF);
    make_dropdown(
        left, refresh_options_text,
        index_of(refresh_options, cfg.refresh_minutes),
        0, 152, refresh_changed_cb
    );

    // Atenuacion por inactividad
    make_label(left, "Atenuar pantalla tras inactividad", 0, 216, 0xFFFFFF);
    make_dropdown(
        left, dim_options_text,
        index_of(dim_options, cfg.dim_minutes),
        0, 242, dim_changed_cb
    );

    lv_obj_t *hint = make_label(
        left,
        "La pantalla recupera el brillo\nal tocarla.",
        0, 300, 0x888888
    );
    (void)hint;

    // --------------------------------------------------
    // Tarjeta derecha: sistema y acciones
    // --------------------------------------------------
    lv_obj_t *right = make_card(screen, 410, 80, 370, 385);

    make_label(right, "Sistema", 0, 0, 0x007ACC, &lv_font_montserrat_16);

    info_label = make_label(right, "", 0, 34, 0xCCCCCC);
    lv_obj_set_width(info_label, 338);
    lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);
    update_info();

    action_status = make_label(right, "", 0, 262, 0xAAAAAA);
    lv_obj_set_width(action_status, 338);
    lv_label_set_long_mode(action_status, LV_LABEL_LONG_WRAP);

    make_button(right, "Olvidar WiFi", 0, 300, 106, 0x333333, forget_wifi_cb);
    make_button(right, "Borrar valores", 116, 300, 116, 0x7A2525, clear_tickers_cb);
    make_button(right, "Reiniciar", 242, 300, 96, 0x333333, restart_cb);

    info_timer = lv_timer_create(info_timer_cb, INFO_REFRESH_MS, nullptr);

    return screen;
}
