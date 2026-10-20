#include "screen_detail.h"
#include "../ui_manager.h"

#include <Arduino.h>
#include "../ui_manager.h"

static void back_btn_event_cb(lv_event_t * e) {
    ui_change_screen(SCREEN_LIST);
}

namespace {
    String selected_symbol = "";
}

void screen_detail_set_symbol(const String &symbol)
{
    selected_symbol = symbol;
}

lv_obj_t* screen_detail_create() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121212), 0);

    // Botón volver
    lv_obj_t *btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, 100, 45);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_add_event_cb(btn_back, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Volver");
    lv_obj_center(lbl_back);

    // Gráfica LVGL
    lv_obj_t *chart = lv_chart_create(scr);
    lv_obj_set_size(chart, 740, 320);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);

    lv_chart_series_t *ser = lv_chart_add_series(chart, lv_color_hex(0x00FF88), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(chart, ser, 100);
    lv_chart_set_next_value(chart, ser, 120);
    lv_chart_set_next_value(chart, ser, 115);
    lv_chart_set_next_value(chart, ser, 140);

    return scr;
}