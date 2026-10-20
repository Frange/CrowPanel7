
#include "screen_detail.h"

#include <Arduino.h>
#include <math.h>

#include "../ui_manager.h"
#include "../../services/market_service.h"

namespace {

String selected_symbol;

lv_obj_t *detail_screen = nullptr;
lv_obj_t *title_label = nullptr;
lv_obj_t *market_label = nullptr;
lv_obj_t *price_label = nullptr;
lv_obj_t *change_label = nullptr;
lv_obj_t *open_label = nullptr;
lv_obj_t *high_label = nullptr;
lv_obj_t *low_label = nullptr;
lv_obj_t *close_label = nullptr;
lv_obj_t *currency_label = nullptr;
lv_obj_t *chart_label = nullptr;
lv_obj_t *chart = nullptr;

lv_obj_t *period_buttons[6] = {};

const char *period_names[] = {
    "1D", "1S", "1M", "6M", "1A", "5A"
};

int selected_period = 0;

lv_obj_t *create_text(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t width,
    lv_color_t color,
    const lv_font_t *font = &lv_font_montserrat_14)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, x, y);

    return label;
}

void back_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_LIST);
}

void update_stock_details()
{
    if (title_label == nullptr) {
        return;
    }

    StockData data = {};

    if (selected_symbol.isEmpty() ||
        !market_service_get_data(selected_symbol, data) ||
        !data.valid) {

        lv_label_set_text(
            title_label,
            selected_symbol.c_str()
        );

        lv_label_set_text(
            market_label,
            "Esperando datos del mercado..."
        );

        lv_label_set_text(price_label, "--");
        lv_label_set_text(change_label, "--");
        lv_label_set_text(open_label, "Apertura: --");
        lv_label_set_text(high_label, "Maximo: --");
        lv_label_set_text(low_label, "Minimo: --");
        lv_label_set_text(close_label, "Cierre anterior: --");
        lv_label_set_text(currency_label, "");

        return;
    }

    // El API actual proporciona el símbolo, no el nombre
    // completo de la empresa.
    lv_label_set_text(
        title_label,
        data.symbol.length() > 0
            ? data.symbol.c_str()
            : selected_symbol.c_str()
    );

    char text[96];

    snprintf(
        text,
        sizeof(text),
        "Mercado: %s",
        data.market.length() > 0
            ? data.market.c_str()
            : "No disponible"
    );
    lv_label_set_text(market_label, text);

    if (isfinite(data.price)) {
        if (data.currency.length() > 0) {
            snprintf(
                text,
                sizeof(text),
                "%.2f %s",
                data.price,
                data.currency.c_str()
            );
        } else {
            snprintf(
                text,
                sizeof(text),
                "%.2f",
                data.price
            );
        }
    } else {
        snprintf(text, sizeof(text), "--");
    }

    lv_label_set_text(price_label, text);

    if (isfinite(data.change_percent)) {
        snprintf(
            text,
            sizeof(text),
            "%+.2f%%",
            data.change_percent
        );

        lv_label_set_text(change_label, text);

        uint32_t color =
            data.change_percent > 0.001f
                ? 0x55DD88
                : data.change_percent < -0.001f
                    ? 0xFF6666
                    : 0xCCCCCC;

        lv_obj_set_style_text_color(
            change_label,
            lv_color_hex(color),
            0
        );
    } else {
        lv_label_set_text(change_label, "--");
    }

    snprintf(
        text, sizeof(text), "Apertura: %.2f",
        isfinite(data.open) ? data.open : 0.0f
    );
    if (!isfinite(data.open)) {
        snprintf(text, sizeof(text), "Apertura: --");
    }
    lv_label_set_text(open_label, text);

    snprintf(
        text, sizeof(text), "Maximo: %.2f",
        isfinite(data.high) ? data.high : 0.0f
    );
    if (!isfinite(data.high)) {
        snprintf(text, sizeof(text), "Maximo: --");
    }
    lv_label_set_text(high_label, text);

    snprintf(
        text, sizeof(text), "Minimo: %.2f",
        isfinite(data.low) ? data.low : 0.0f
    );
    if (!isfinite(data.low)) {
        snprintf(text, sizeof(text), "Minimo: --");
    }
    lv_label_set_text(low_label, text);

    snprintf(
        text, sizeof(text), "Cierre anterior: %.2f",
        isfinite(data.previous_close)
            ? data.previous_close
            : 0.0f
    );
    if (!isfinite(data.previous_close)) {
        snprintf(text, sizeof(text), "Cierre anterior: --");
    }
    lv_label_set_text(close_label, text);

    snprintf(
        text,
        sizeof(text),
        "Variacion absoluta: %+.2f",
        isfinite(data.change) ? data.change : 0.0f
    );
    lv_label_set_text(currency_label, text);
}

void period_btn_event_cb(lv_event_t *e)
{
    int period = static_cast<int>(
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(e)
        )
    );

    if (period < 0 || period >= 6) {
        return;
    }

    selected_period = period;

    for (int i = 0; i < 6; ++i) {
        if (period_buttons[i] == nullptr) {
            continue;
        }

        lv_obj_set_style_bg_color(
            period_buttons[i],
            i == selected_period
                ? lv_color_hex(0x007ACC)
                : lv_color_hex(0x303030),
            LV_PART_MAIN
        );
    }

    // Los botones ya seleccionan el período visualmente.
    // Falta conectar una API de histórico para dibujar
    // los precios reales del intervalo elegido.
    if (chart_label != nullptr) {
        char message[96];

        snprintf(
            message,
            sizeof(message),
            "Historico %s pendiente de conectar",
            period_names[selected_period]
        );

        lv_label_set_text(chart_label, message);
    }
}

} // namespace

void screen_detail_set_symbol(const String &symbol)
{
    selected_symbol = symbol;
}

lv_obj_t *screen_detail_create()
{
    detail_screen = lv_obj_create(nullptr);

    lv_obj_set_style_bg_color(
        detail_screen,
        lv_color_hex(0x121212),
        0
    );

    lv_obj_set_style_border_width(detail_screen, 0, 0);
    lv_obj_clear_flag(detail_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Barra superior
    lv_obj_t *back = lv_btn_create(detail_screen);
    lv_obj_set_size(back, 100, 42);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 12, 10);

    lv_obj_set_style_bg_color(
        back, lv_color_hex(0x333333), 0
    );

    lv_obj_add_event_cb(
        back,
        back_btn_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t *back_text = lv_label_create(back);
    lv_label_set_text(back_text, LV_SYMBOL_LEFT " Volver");
    lv_obj_center(back_text);

    title_label = create_text(
        detail_screen, selected_symbol.c_str(),
        130, 12, 350, lv_color_hex(0xFFFFFF),
        &lv_font_montserrat_14
    );

    market_label = create_text(
        detail_screen, "Mercado: --",
        130, 40, 350, lv_color_hex(0xAAAAAA)
    );

    // Tarjeta de cotización
    lv_obj_t *quote = lv_obj_create(detail_screen);
    lv_obj_set_size(quote, 760, 116);
    lv_obj_align(quote, LV_ALIGN_TOP_MID, 0, 65);

    lv_obj_set_style_bg_color(
        quote, lv_color_hex(0x202020), 0
    );
    lv_obj_set_style_border_width(quote, 0, 0);
    lv_obj_set_style_radius(quote, 8, 0);
    lv_obj_set_style_pad_all(quote, 10, 0);
    lv_obj_clear_flag(quote, LV_OBJ_FLAG_SCROLLABLE);

    price_label = create_text(
        quote, "--", 12, 8, 260,
        lv_color_hex(0xFFFFFF), &lv_font_montserrat_14
    );

    change_label = create_text(
        quote, "--", 12, 43, 200,
        lv_color_hex(0x55DD88), &lv_font_montserrat_14
    );

    open_label = create_text(
        quote, "Apertura: --", 310, 8, 200,
        lv_color_hex(0xCCCCCC)
    );

    high_label = create_text(
        quote, "Maximo: --", 310, 34, 200,
        lv_color_hex(0xCCCCCC)
    );

    low_label = create_text(
        quote, "Minimo: --", 310, 60, 200,
        lv_color_hex(0xCCCCCC)
    );

    close_label = create_text(
        quote, "Cierre anterior: --", 520, 8, 220,
        lv_color_hex(0xCCCCCC)
    );

    currency_label = create_text(
        quote, "", 12, 76, 260,
        lv_color_hex(0xAAAAAA)
    );

    // Selector de intervalo
    for (int i = 0; i < 6; ++i) {
        period_buttons[i] = lv_btn_create(detail_screen);

        lv_obj_set_size(period_buttons[i], 82, 36);

        lv_obj_align(
            period_buttons[i],
            LV_ALIGN_TOP_LEFT,
            20 + i * 91,
            194
        );

        lv_obj_set_style_bg_color(
            period_buttons[i],
            i == selected_period
                ? lv_color_hex(0x007ACC)
                : lv_color_hex(0x303030),
            LV_PART_MAIN
        );

        lv_obj_add_event_cb(
            period_buttons[i],
            period_btn_event_cb,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(
                static_cast<uintptr_t>(i)
            )
        );

        lv_obj_t *label = lv_label_create(period_buttons[i]);
        lv_label_set_text(label, period_names[i]);
        lv_obj_center(label);
    }

    // Gráfica
    chart = lv_chart_create(detail_screen);

    lv_obj_set_size(chart, 760, 205);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 238);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 2);
    lv_chart_set_range(
        chart,
        LV_CHART_AXIS_PRIMARY_Y,
        0,
        100
    );

    // Evitar que la cuadrícula predeterminada domine la vista.
    lv_obj_set_style_bg_color(
        chart, lv_color_hex(0x181818), LV_PART_MAIN
    );
    lv_obj_set_style_bg_opa(
        chart, LV_OPA_COVER, LV_PART_MAIN
    );
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(
        chart, lv_color_hex(0x303030), LV_PART_MAIN
    );
    lv_chart_set_div_line_count(chart, 0, 0);

    chart_label = lv_label_create(chart);
    lv_label_set_text(
        chart_label,
        "El historico de precios aun no esta conectado"
    );
    lv_obj_set_width(chart_label, 700);
    lv_obj_set_style_text_align(
        chart_label, LV_TEXT_ALIGN_CENTER, 0
    );
    lv_obj_set_style_text_color(
        chart_label, lv_color_hex(0xAAAAAA), 0
    );
    lv_obj_align(chart_label, LV_ALIGN_CENTER, 0, 0);

    update_stock_details();

    return detail_screen;
}
