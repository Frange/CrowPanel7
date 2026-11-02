#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
#include <stdio.h>

#include "screen_list_internal.h"

#include "../../../services/storage_service.h"
#include "../../../services/market_service.h"
#include "../../../services/settings_service.h"

namespace {

void format_price(
    char *out,
    size_t size,
    float value,
    const String &currency)
{
    if (!isfinite(value)) {
        snprintf(out, size, "--");
    } else if (!currency.isEmpty()) {
        snprintf(out, size, "%.2f %s", value, currency.c_str());
    } else {
        snprintf(out, size, "%.2f", value);
    }
}

void set_row_placeholder(ScreenListState &state, size_t i)
{
    screen_list_set_text(state.price_labels[i], "--");
    screen_list_set_text(state.open_labels[i], "--");
    screen_list_set_text(state.close_labels[i], "--");
    screen_list_set_text(state.change_labels[i], "--");

    lv_obj_set_style_text_color(
        state.change_labels[i], lv_color_hex(0xAAAAAA), 0
    );
}

} // namespace

void screen_list_update_wifi_indicator()
{
    auto &state = screen_list_state;

    if (state.wifi_indicator == nullptr) {
        return;
    }

    int connected = WiFi.status() == WL_CONNECTED ? 1 : 0;

    if (connected == state.painted_wifi) {
        return;
    }

    state.painted_wifi = connected;

    if (connected) {
        lv_label_set_text(state.wifi_indicator, LV_SYMBOL_WIFI " Conectado");
        lv_obj_set_style_text_color(
            state.wifi_indicator, lv_color_hex(0x55DD88), 0
        );
    } else {
        lv_label_set_text(state.wifi_indicator, LV_SYMBOL_WIFI " Sin WiFi");
        lv_obj_set_style_text_color(
            state.wifi_indicator, lv_color_hex(0xFFCC66), 0
        );
    }
}

void screen_list_refresh_status()
{
    auto &state = screen_list_state;

    if (state.refresh_status == nullptr) {
        return;
    }

    char text[96];

    if (WiFi.status() != WL_CONNECTED) {
        snprintf(text, sizeof(text), "Sin conexion WiFi");
    } else if (market_service_is_busy()) {
        snprintf(text, sizeof(text), "Actualizando cotizaciones...");
    } else {
        uint32_t last = market_service_last_refresh_ms();
        unsigned int every = settings_get().refresh_minutes;

        if (last == 0) {
            snprintf(
                text, sizeof(text),
                "Actualizacion automatica cada %u min", every
            );
        } else {
            uint32_t ago_s = (millis() - last) / 1000;

            if (ago_s < 60) {
                snprintf(
                    text, sizeof(text),
                    "Actualizado hace %lu s  -  automatico cada %u min",
                    (unsigned long)ago_s, every
                );
            } else {
                snprintf(
                    text, sizeof(text),
                    "Actualizado hace %lu min  -  automatico cada %u min",
                    (unsigned long)(ago_s / 60), every
                );
            }
        }
    }

    screen_list_set_text(state.refresh_status, text);
}

void screen_list_update_rows()
{
    auto &state = screen_list_state;

    size_t count = storage_ticker_count();

    if (count > MAX_ROWS) {
        count = MAX_ROWS;
    }

    for (size_t i = 0; i < MAX_ROWS; ++i) {
        lv_obj_t *row = state.rows[i];

        if (row == nullptr) {
            continue;
        }

        if (i >= count) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);

        String symbol = storage_get_ticker(i);
        screen_list_set_text(state.ticker_labels[i], symbol.c_str());

        StockData data = {};

        if (symbol.isEmpty() ||
            !market_service_get_data(symbol, data) ||
            !data.valid) {
            set_row_placeholder(state, i);
            continue;
        }

        char text[48];

        format_price(text, sizeof(text), data.price, data.currency);
        screen_list_set_text(state.price_labels[i], text);

        format_price(text, sizeof(text), data.open, data.currency);
        screen_list_set_text(state.open_labels[i], text);

        format_price(text, sizeof(text), data.previous_close, data.currency);
        screen_list_set_text(state.close_labels[i], text);

        // Variacion monetaria y porcentual en una sola linea.
        if (isfinite(data.change) && isfinite(data.change_percent)) {
            if (!data.currency.isEmpty()) {
                snprintf(
                    text, sizeof(text), "%+.2f %s (%+.2f%%)",
                    data.change, data.currency.c_str(), data.change_percent
                );
            } else {
                snprintf(
                    text, sizeof(text), "%+.2f (%+.2f%%)",
                    data.change, data.change_percent
                );
            }

            screen_list_set_text(state.change_labels[i], text);

            uint32_t color =
                data.change > 0.001f ? 0x55DD88 :
                data.change < -0.001f ? 0xFF6666 :
                0xCCCCCC;

            lv_obj_set_style_text_color(
                state.change_labels[i], lv_color_hex(color), 0
            );
        } else {
            screen_list_set_text(state.change_labels[i], "--");
            lv_obj_set_style_text_color(
                state.change_labels[i], lv_color_hex(0xAAAAAA), 0
            );
        }
    }

    if (state.empty_label != nullptr) {
        if (count == 0) {
            lv_obj_clear_flag(state.empty_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(state.empty_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
