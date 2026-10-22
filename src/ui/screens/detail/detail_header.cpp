#include "detail_header.h"

#include "../../ui_manager.h"

namespace {

lv_obj_t *title_label = nullptr;
lv_obj_t *market_label = nullptr;

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

} // namespace

lv_obj_t *detail_header_create(
    lv_obj_t *parent,
    const String &symbol)
{
    lv_obj_t *back = lv_btn_create(parent);
    lv_obj_set_size(back, 100, 42);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 12, 10);
    lv_obj_set_style_bg_color(
        back,
        lv_color_hex(0x333333),
        0
    );

    lv_obj_add_event_cb(
        back,
        back_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(
        back_label,
        LV_SYMBOL_LEFT " Volver"
    );
    lv_obj_center(back_label);

    title_label = make_label(
        parent,
        symbol.c_str(),
        130,
        8,
        400,
        lv_color_hex(0xFFFFFF)
    );

    market_label = make_label(
        parent,
        "Mercado: --",
        130,
        34,
        500,
        lv_color_hex(0xAAAAAA)
    );

    return title_label;
}

void detail_header_set_symbol(const String &symbol)
{
    if (title_label == nullptr) {
        return;
    }

    lv_obj_t *screen = lv_obj_get_screen(title_label);

    if (screen == nullptr || screen != lv_scr_act()) {
        return;
    }

    lv_label_set_text(title_label, symbol.c_str());
}

void detail_header_reset()
{
    title_label = nullptr;
    market_label = nullptr;
}