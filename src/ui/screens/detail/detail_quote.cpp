#include "detail_quote.h"

#include <math.h>
#include <stdio.h>

namespace {

lv_obj_t *market_label = nullptr;
lv_obj_t *price_label = nullptr;
lv_obj_t *change_label = nullptr;
lv_obj_t *open_label = nullptr;
lv_obj_t *high_label = nullptr;
lv_obj_t *low_label = nullptr;
lv_obj_t *close_label = nullptr;
lv_obj_t *variation_label = nullptr;

lv_obj_t *make_label(
    lv_obj_t *parent,
    const char *text,
    int x,
    int y,
    int width,
    lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_14,
        0
    );
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, x, y);
    return label;
}

void set_label(lv_obj_t *label, const char *text)
{
    if (label != nullptr) {
        lv_label_set_text(label, text);
    }
}

void format_price(
    char *out,
    size_t size,
    float value,
    const String &currency)
{
    if (!isfinite(value)) {
        snprintf(out, size, "--");
    } else if (!currency.isEmpty()) {
        snprintf(
            out,
            size,
            "%.2f %s",
            value,
            currency.c_str()
        );
    } else {
        snprintf(out, size, "%.2f", value);
    }
}

void format_field(
    char *out,
    size_t size,
    const char *name,
    float value,
    const String &currency)
{
    char price[48];
    format_price(price, sizeof(price), value, currency);
    snprintf(out, size, "%s: %s", name, price);
}

} // namespace

lv_obj_t *detail_quote_create(lv_obj_t *parent)
{
    lv_obj_t *quote = lv_obj_create(parent);
    lv_obj_set_size(quote, 760, 116);
    lv_obj_align(
        quote,
        LV_ALIGN_TOP_MID,
        0,
        65
    );

    lv_obj_set_style_bg_color(
        quote,
        lv_color_hex(0x202020),
        0
    );
    lv_obj_set_style_border_width(quote, 0, 0);
    lv_obj_set_style_radius(quote, 8, 0);
    lv_obj_set_style_pad_all(quote, 10, 0);
    lv_obj_clear_flag(quote, LV_OBJ_FLAG_SCROLLABLE);

    market_label = make_label(
        quote, "Mercado: --", 12, 0, 700,
        lv_color_hex(0xAAAAAA)
    );

    price_label = make_label(
        quote, "--", 12, 22, 270,
        lv_color_hex(0xFFFFFF)
    );

    change_label = make_label(
        quote, "--", 12, 47, 180,
        lv_color_hex(0x55DD88)
    );

    variation_label = make_label(
        quote, "Variacion: --", 12, 73, 270,
        lv_color_hex(0xAAAAAA)
    );

    open_label = make_label(
        quote, "Apertura: --", 300, 8, 220,
        lv_color_hex(0xCCCCCC)
    );

    high_label = make_label(
        quote, "Maximo: --", 300, 34, 220,
        lv_color_hex(0xCCCCCC)
    );

    low_label = make_label(
        quote, "Minimo: --", 300, 60, 220,
        lv_color_hex(0xCCCCCC)
    );

    close_label = make_label(
        quote, "Cierre anterior: --", 520, 8, 225,
        lv_color_hex(0xCCCCCC)
    );

    return quote;
}

void detail_quote_update(
    const StockData &data,
    bool available)
{
    if (!available || !data.valid) {
        set_label(market_label, "Mercado: no disponible");
        set_label(price_label, "--");
        set_label(change_label, "--");
        set_label(open_label, "Apertura: --");
        set_label(high_label, "Maximo: --");
        set_label(low_label, "Minimo: --");
        set_label(close_label, "Cierre anterior: --");
        set_label(variation_label, "Variacion: --");

        lv_obj_set_style_text_color(
            change_label,
            lv_color_hex(0xAAAAAA),
            0
        );
        return;
    }

    char text[112];

    snprintf(
        text,
        sizeof(text),
        "Mercado: %s",
        data.market.isEmpty()
            ? "No disponible"
            : data.market.c_str()
    );
    set_label(market_label, text);

    format_price(
        text, sizeof(text), data.price, data.currency
    );
    set_label(price_label, text);

    if (isfinite(data.change_percent)) {
        snprintf(
            text, sizeof(text), "%+.2f%%",
            data.change_percent
        );
        set_label(change_label, text);

        uint32_t color =
            data.change_percent > 0.001f ? 0x55DD88 :
            data.change_percent < -0.001f ? 0xFF6666 :
            0xCCCCCC;

        lv_obj_set_style_text_color(
            change_label,
            lv_color_hex(color),
            0
        );
    } else {
        set_label(change_label, "--");
        lv_obj_set_style_text_color(
            change_label,
            lv_color_hex(0xAAAAAA),
            0
        );
    }

    format_field(
        text, sizeof(text), "Apertura", data.open,
        data.currency
    );
    set_label(open_label, text);

    format_field(
        text, sizeof(text), "Maximo", data.high,
        data.currency
    );
    set_label(high_label, text);

    format_field(
        text, sizeof(text), "Minimo", data.low,
        data.currency
    );
    set_label(low_label, text);

    format_field(
        text, sizeof(text), "Cierre anterior",
        data.previous_close, data.currency
    );
    set_label(close_label, text);

    format_field(
        text, sizeof(text), "Variacion",
        data.change, data.currency
    );
    set_label(variation_label, text);
}