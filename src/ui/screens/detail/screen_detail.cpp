#include "screen_detail.h"

#include <Arduino.h>

#include "../../ui_manager.h"
#include "../../../services/market_service.h"

#include "detail_header.h"
#include "detail_quote.h"
#include "detail_chart.h"

namespace {

String selected_symbol;

lv_obj_t *detail_screen = nullptr;

// --------------------------------------------------
// Actualizar la cotización desde la caché
// --------------------------------------------------

void update_stock_details()
{
    StockData data = {};

    bool available =
        !selected_symbol.isEmpty() &&
        market_service_get_data(
            selected_symbol,
            data
        ) &&
        data.valid;

    detail_quote_update(
        data,
        available
    );
}

} // namespace

// --------------------------------------------------
// Establecer símbolo seleccionado
// --------------------------------------------------

void screen_detail_set_symbol(const String &symbol)
{
    selected_symbol = symbol;
}

// --------------------------------------------------
// Crear pantalla de detalle
// --------------------------------------------------

lv_obj_t *screen_detail_create()
{
    detail_screen = lv_obj_create(nullptr);

    // Fondo de pantalla.
    lv_obj_set_style_bg_color(
        detail_screen,
        lv_color_hex(0x121212),
        0
    );

    lv_obj_set_style_border_width(
        detail_screen,
        0,
        0
    );

    lv_obj_clear_flag(
        detail_screen,
        LV_OBJ_FLAG_SCROLLABLE
    );

    // --------------------------------------------------
    // Cabecera: botón volver, símbolo y mercado
    // --------------------------------------------------

    detail_header_create(
        detail_screen,
        selected_symbol
    );

    // --------------------------------------------------
    // Panel de cotización
    // --------------------------------------------------

    detail_quote_create(
        detail_screen
    );

    // --------------------------------------------------
    // Histórico y gráfico
    // --------------------------------------------------

    detail_chart_create(
        detail_screen,
        selected_symbol
    );

    // --------------------------------------------------
    // Datos iniciales de la cotización
    // --------------------------------------------------

    update_stock_details();

    return detail_screen;
}