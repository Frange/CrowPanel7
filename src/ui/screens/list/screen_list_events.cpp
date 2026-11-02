#include <Arduino.h>

#include "screen_list_internal.h"

#include "../../ui_manager.h"
#include "../../ui_dialog.h"
#include "../detail/screen_detail.h"

#include "../../../services/storage_service.h"
#include "../../../services/market_service.h"

namespace {

size_t index_from_event(lv_event_t *e)
{
    return static_cast<size_t>(
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(e))
    );
}

void delete_confirmed(void *user_data)
{
    size_t index = reinterpret_cast<uintptr_t>(user_data);

    String symbol = storage_get_ticker(index);

    if (symbol.isEmpty() || !storage_remove_ticker(index)) {
        Serial.printf(
            "[UI] Error al eliminar el ticker %s\n", symbol.c_str()
        );
        return;
    }

    Serial.printf("[UI] Ticker %s eliminado\n", symbol.c_str());

    // Las filas se reasignan por indice; basta con repintar.
    screen_list_state.painted_count = storage_ticker_count();
    screen_list_update_rows();
}

} // namespace

void screen_list_ticker_row_event_cb(lv_event_t *e)
{
    size_t index = index_from_event(e);

    if (index >= MAX_ROWS) {
        return;
    }

    String symbol = storage_normalize_ticker(storage_get_ticker(index));

    if (symbol.isEmpty()) {
        return;
    }

    StockData data = {};

    if (!market_service_get_data(symbol, data) || !data.valid) {
        Serial.printf(
            "[LIST] No se abre %s: sin datos validos\n", symbol.c_str()
        );

        screen_list_set_text(
            screen_list_state.refresh_status,
            "Esperando datos de la cotizacion..."
        );

        market_service_request_refresh();
        return;
    }

    screen_detail_set_symbol(symbol);
    ui_change_screen(SCREEN_DETAIL);
}

void screen_list_refresh_btn_event_cb(lv_event_t *e)
{
    (void)e;

    if (market_service_request_refresh()) {
        screen_list_set_text(
            screen_list_state.refresh_status,
            "Actualizando cotizaciones..."
        );
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
    size_t index = index_from_event(e);

    if (index >= MAX_ROWS) {
        return;
    }

    String symbol = storage_get_ticker(index);

    if (symbol.isEmpty()) {
        return;
    }

    static char message[64];
    snprintf(
        message, sizeof(message),
        "Se quitara %s de la lista.", symbol.c_str()
    );

    ui_confirm(
        "Eliminar valor",
        message,
        "Eliminar",
        true,
        delete_confirmed,
        reinterpret_cast<void *>(static_cast<uintptr_t>(index))
    );
}
