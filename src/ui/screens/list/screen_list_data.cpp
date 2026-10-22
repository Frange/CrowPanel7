
#include <Arduino.h>
#include <WiFi.h>
#include <math.h>

#include "screen_list_internal.h"
#include "screen_list_data.h"

#include "../../../services/storage_service.h"
#include "../../../services/market_service.h"

void screen_list_update_wifi_indicator()
{
    auto &state = screen_list_state;

    if (state.wifi_indicator == nullptr) {
        return;
    }

    bool connected = WiFi.status() == WL_CONNECTED;

    if (connected) {
        lv_label_set_text(
            state.wifi_indicator,
            LV_SYMBOL_WIFI " Conectado"
        );

        lv_obj_set_style_text_color(
            state.wifi_indicator,
            lv_color_hex(0x55DD88),
            0
        );

        // Solicitar cotizaciones al detectar la conexión.
        if (!state.wifi_was_connected) {
            state.wifi_was_connected = true;

            Serial.println(
                "[LIST] WiFi conectado; solicitando cotizaciones"
            );

            market_service_request_refresh();
        }
    } else {
        lv_label_set_text(
            state.wifi_indicator,
            LV_SYMBOL_WIFI " Sin WiFi"
        );

        lv_obj_set_style_text_color(
            state.wifi_indicator,
            lv_color_hex(0xFFCC66),
            0
        );

        state.wifi_was_connected = false;
    }
}

void screen_list_refresh_status()
{
    auto &state = screen_list_state;

    if (state.refresh_status == nullptr) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        lv_label_set_text(
            state.refresh_status,
            "Sin conexion WiFi"
        );
    } else if (market_service_is_busy()) {
        lv_label_set_text(
            state.refresh_status,
            "Actualizando cotizaciones..."
        );
    } else {
        lv_label_set_text(
            state.refresh_status,
            "Actualizacion automatica cada 5 min"
        );
    }
}

void screen_list_update_rows()
{
    auto &state = screen_list_state;

    size_t count = storage_ticker_count();

    if (count > MAX_ROWS) {
        count = MAX_ROWS;
    }

    for (size_t i = 0; i < MAX_ROWS; ++i) {
        if (state.ticker_labels[i] == nullptr) {
            continue;
        }

        lv_obj_t *row =
            lv_obj_get_parent(state.ticker_labels[i]);

        if (i >= count) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);

        String symbol = storage_get_ticker(i);
        StockData data = {};

        if (symbol.isEmpty() ||
            !market_service_get_data(symbol, data)) {

            lv_label_set_text(
                state.ticker_labels[i],
                symbol.c_str()
            );
            lv_label_set_text(state.price_labels[i], "--");
            lv_label_set_text(state.open_labels[i], "--");
            lv_label_set_text(state.close_labels[i], "--");
            lv_label_set_text(state.change_labels[i], "--");

            if (state.change_amount_labels[i] != nullptr) {
                lv_label_set_text(
                    state.change_amount_labels[i],
                    "--"
                );
            }

            continue;
        }

        lv_label_set_text(
            state.ticker_labels[i],
            symbol.c_str()
        );

        char price_text[64];
        char open_text[64];
        char close_text[64];
        char change_text[32];
        char amount_text[64];

        // --------------------------------------------------
        // Precio actual
        // --------------------------------------------------
        if (isfinite(data.price)) {
            if (!data.currency.isEmpty()) {
                snprintf(
                    price_text,
                    sizeof(price_text),
                    "%.2f %s",
                    data.price,
                    data.currency.c_str()
                );
            } else {
                snprintf(
                    price_text,
                    sizeof(price_text),
                    "%.2f",
                    data.price
                );
            }
        } else {
            snprintf(price_text, sizeof(price_text), "--");
        }

        // --------------------------------------------------
        // Precio de apertura
        // --------------------------------------------------
        if (isfinite(data.open)) {
            if (!data.currency.isEmpty()) {
                snprintf(
                    open_text,
                    sizeof(open_text),
                    "%.2f %s",
                    data.open,
                    data.currency.c_str()
                );
            } else {
                snprintf(
                    open_text,
                    sizeof(open_text),
                    "%.2f",
                    data.open
                );
            }
        } else {
            snprintf(open_text, sizeof(open_text), "--");
        }

        // --------------------------------------------------
        // Cierre anterior
        // --------------------------------------------------
        if (isfinite(data.previous_close)) {
            if (!data.currency.isEmpty()) {
                snprintf(
                    close_text,
                    sizeof(close_text),
                    "%.2f %s",
                    data.previous_close,
                    data.currency.c_str()
                );
            } else {
                snprintf(
                    close_text,
                    sizeof(close_text),
                    "%.2f",
                    data.previous_close
                );
            }
        } else {
            snprintf(close_text, sizeof(close_text), "--");
        }

        lv_label_set_text(
            state.price_labels[i],
            price_text
        );

        lv_label_set_text(
            state.open_labels[i],
            open_text
        );

        lv_label_set_text(
            state.close_labels[i],
            close_text
        );

        // --------------------------------------------------
// Variación monetaria y porcentual en una sola línea
// --------------------------------------------------
if (isfinite(data.change_percent) &&
    isfinite(data.price) &&
    isfinite(data.previous_close)) {

    const float difference =
        data.price - data.previous_close;

    if (!data.currency.isEmpty()) {
        snprintf(
            change_text,
            sizeof(change_text),
            "%+.2f %s (%+.2f%%)",
            difference,
            data.currency.c_str(),
            data.change_percent
        );
    } else {
        snprintf(
            change_text,
            sizeof(change_text),
            "%+.2f (%+.2f%%)",
            difference,
            data.change_percent
        );
    }

    lv_label_set_text(
        state.change_labels[i],
        change_text
    );

    uint32_t color =
        difference > 0.001f
            ? 0x55DD88
            : difference < -0.001f
                ? 0xFF6666
                : 0xCCCCCC;

    lv_obj_set_style_text_color(
        state.change_labels[i],
        lv_color_hex(color),
        0
    );

} else {
    lv_label_set_text(
        state.change_labels[i],
        "--"
    );

    lv_obj_set_style_text_color(
        state.change_labels[i],
        lv_color_hex(0xAAAAAA),
        0
    );
}
    }

    // --------------------------------------------------
    // Mensaje cuando no hay valores guardados
    // --------------------------------------------------
    if (state.empty_label != nullptr) {
        if (count == 0) {
            lv_obj_clear_flag(
                state.empty_label,
                LV_OBJ_FLAG_HIDDEN
            );
        } else {
            lv_obj_add_flag(
                state.empty_label,
                LV_OBJ_FLAG_HIDDEN
            );
        }
    }

    screen_list_refresh_status();
}
