
#include <Arduino.h>

#include "screen_list_internal.h"
#include "screen_list_data.h"

#include "../../ui_manager.h"
#include "../detail/screen_detail.h"

#include "../../../services/storage_service.h"
#include "../../../services/market_service.h"


void screen_list_ticker_row_event_cb(lv_event_t *e)
{
    size_t index = static_cast<size_t>(
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(e)
        )
    );

    if (index >= MAX_ROWS) {
        return;
    }

    String symbol = storage_get_ticker(index);
    symbol.trim();
    symbol.toUpperCase();

    if (symbol.isEmpty()) {
        return;
    }

    // No abrir el detalle si no tenemos una cotización válida.
    StockData data = {};

    if (!market_service_get_data(symbol, data) ||
        !data.valid) {

        Serial.printf(
            "[LIST] No se abre %s: no hay datos de cotizacion validos\n",
            symbol.c_str()
        );

        if (screen_list_state.refresh_status != nullptr) {
            lv_label_set_text(
                screen_list_state.refresh_status,
                "Esperando datos validos de la cotizacion..."
            );
        }

        // Si no hay datos, solicitamos una actualización.
        // Si ya hay una en curso, el servicio puede rechazarla.
        market_service_request_refresh();

        return;
    }

    Serial.printf(
        "[LIST] Abriendo detalle de %s (precio=%.4f)\n",
        symbol.c_str(),
        data.price
    );

    screen_detail_set_symbol(symbol);
    ui_change_screen(SCREEN_DETAIL);
}


void screen_list_refresh_btn_event_cb(lv_event_t *e)
{
    (void)e;

    if (market_service_request_refresh()) {
        screen_list_state.last_refresh_request = millis();

        if (screen_list_state.refresh_status != nullptr) {
            lv_label_set_text(
                screen_list_state.refresh_status,
                "Actualizando cotizaciones..."
            );
        }
    }
}

void screen_list_add_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_ADD);
}

void screen_list_settings_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_SETTINGS);
}

void screen_list_delete_btn_event_cb(lv_event_t *e)
{
    size_t index = static_cast<size_t>(
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(e)
        )
    );

    if (index >= MAX_ROWS) {
        return;
    }

    String symbol = storage_get_ticker(index);

    if (symbol.isEmpty()) {
        return;
    }

    if (!storage_remove_ticker(index)) {
        Serial.printf(
            "[UI] Error al eliminar el ticker %s\n",
            symbol.c_str()
        );
        return;
    }

    Serial.printf(
        "[UI] Ticker %s eliminado correctamente\n",
        symbol.c_str()
    );

    size_t count = storage_ticker_count();

    if (count > MAX_ROWS) {
        count = MAX_ROWS;
    }

    auto &state = screen_list_state;

    for (size_t i = 0; i < MAX_ROWS; ++i) {
        if (state.ticker_labels[i] == nullptr) {
            continue;
        }

        lv_obj_t *row =
            lv_obj_get_parent(state.ticker_labels[i]);

        if (i < count) {
            String remaining_symbol = storage_get_ticker(i);

            lv_label_set_text(
                state.ticker_labels[i],
                remaining_symbol.c_str()
            );

            lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);

            lv_obj_align(
                row,
                LV_ALIGN_TOP_LEFT,
                0,
                static_cast<lv_coord_t>(i * 53)
            );
        } else {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
        }
    }

    screen_list_update_rows();
}
