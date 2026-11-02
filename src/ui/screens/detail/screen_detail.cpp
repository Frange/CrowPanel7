#include "screen_detail.h"

#include <Arduino.h>

#include "../../ui_manager.h"
#include "../../../services/market_service.h"

#include "detail_header.h"
#include "detail_quote.h"
#include "detail_chart.h"

namespace {

constexpr uint32_t QUOTE_POLL_MS = 1000;

String selected_symbol;

lv_obj_t *detail_screen = nullptr;
lv_timer_t *quote_timer = nullptr;
uint32_t painted_version = 0xFFFFFFFFu;

void update_stock_details()
{
    StockData data = {};

    bool available =
        !selected_symbol.isEmpty() &&
        market_service_get_data(selected_symbol, data) &&
        data.valid;

    detail_header_set_info(data, available);
    detail_quote_update(data, available);
}

// Repinta la cotizacion cuando llega una actualizacion automatica
// mientras el detalle esta abierto.
void quote_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    uint32_t version = market_service_data_version();

    if (version != painted_version) {
        painted_version = version;
        update_stock_details();
    }
}

void screen_delete_cb(lv_event_t *e)
{
    if (lv_event_get_target(e) != detail_screen) {
        return;
    }

    if (quote_timer != nullptr) {
        lv_timer_del(quote_timer);
        quote_timer = nullptr;
    }

    detail_chart_destroy();
    detail_header_reset();
    detail_quote_reset();

    detail_screen = nullptr;
}

} // namespace

void screen_detail_set_symbol(const String &symbol)
{
    selected_symbol = symbol;
}

lv_obj_t *screen_detail_create()
{
    detail_screen = lv_obj_create(nullptr);

    lv_obj_set_style_bg_color(detail_screen, lv_color_hex(0x121212), 0);
    lv_obj_set_style_border_width(detail_screen, 0, 0);
    lv_obj_clear_flag(detail_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(
        detail_screen, screen_delete_cb, LV_EVENT_DELETE, nullptr
    );

    detail_header_create(detail_screen, selected_symbol);
    detail_quote_create(detail_screen);
    detail_chart_create(detail_screen, selected_symbol);

    painted_version = market_service_data_version();
    update_stock_details();

    quote_timer = lv_timer_create(quote_timer_cb, QUOTE_POLL_MS, nullptr);

    return detail_screen;
}
