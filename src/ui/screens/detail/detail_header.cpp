#include "detail_header.h"

#include "../../ui_manager.h"

namespace {

lv_obj_t *title_label = nullptr;
lv_obj_t *subtitle_label = nullptr;

void back_event_cb(lv_event_t *event)
{
    (void)event;
    ui_change_screen(SCREEN_LIST);
}

lv_obj_t *make_label(
    lv_obj_t *parent,
    const char *text,
    int x,
    int y,
    int width,
    lv_color_t color,
    const lv_font_t *font)
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

} // namespace

lv_obj_t *detail_header_create(
    lv_obj_t *parent,
    const String &symbol)
{
    lv_obj_t *back = lv_btn_create(parent);
    lv_obj_set_size(back, 100, 42);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 12, 10);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back, 8, 0);
    lv_obj_add_event_cb(back, back_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Volver");
    lv_obj_center(back_label);

    title_label = make_label(
        parent, symbol.c_str(), 130, 8, 400,
        lv_color_hex(0xFFFFFF), &lv_font_montserrat_20
    );

    subtitle_label = make_label(
        parent, "", 130, 36, 640,
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14
    );

    return title_label;
}

void detail_header_set_info(const StockData &data, bool available)
{
    if (subtitle_label == nullptr) {
        return;
    }

    if (!available) {
        lv_label_set_text(subtitle_label, "Sin datos de la cotizacion");
        return;
    }

    if (!data.name.isEmpty() && !data.market.isEmpty()) {
        lv_label_set_text_fmt(
            subtitle_label, "%s  -  %s",
            data.name.c_str(), data.market.c_str()
        );
    } else if (!data.name.isEmpty()) {
        lv_label_set_text(subtitle_label, data.name.c_str());
    } else {
        lv_label_set_text_fmt(
            subtitle_label, "Mercado: %s", data.market.c_str()
        );
    }
}

void detail_header_reset()
{
    title_label = nullptr;
    subtitle_label = nullptr;
}
