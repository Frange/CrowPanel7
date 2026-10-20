#include "screen_add.h"
#include "../ui_manager.h"

static void back_btn_event_cb(lv_event_t * e) {
    (void)e;
    ui_change_screen(SCREEN_LIST);
}

static void save_btn_event_cb(lv_event_t * e) {
    (void)e;
    // Guardar ticker y volver a la lista
    ui_change_screen(SCREEN_LIST);
}

lv_obj_t* screen_add_create() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121212), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Top Bar
    lv_obj_t *top_bar = lv_obj_create(scr);
    lv_obj_set_size(top_bar, 800, 65);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1F1F1F), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(top_bar);
    lv_obj_set_size(btn_back, 90, 42);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 15, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(btn_back, 8, 0);
    lv_obj_add_event_cb(btn_back, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Volver");
    lv_obj_center(lbl_back);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Anadir Nueva Accion / ETF");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Contenido central (Formulario)
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 600, 350);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_width(content, 1, 0);
    lv_obj_set_style_border_color(content, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(content, 16, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_info = lv_label_create(content);
    lv_label_set_text(lbl_info, "Introduce el simbolo del ticker (ej. AAPL, TSLA, SPY):");
    lv_obj_set_style_text_color(lbl_info, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl_info, LV_ALIGN_TOP_MID, 0, 30);

    // Caja de texto simulada / input
    lv_obj_t *ta = lv_textarea_create(content);
    lv_obj_set_size(ta, 350, 55);
    lv_obj_align(ta, LV_ALIGN_CENTER, 0, -10);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "Ej: AAPL");

    // Boton Guardar
    lv_obj_t *btn_save = lv_btn_create(content);
    lv_obj_set_size(btn_save, 200, 50);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x007ACC), 0);
    lv_obj_set_style_radius(btn_save, 10, 0);
    lv_obj_add_event_cb(btn_save, save_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "Guardar Ticker");
    lv_obj_center(lbl_save);

    return scr;
}