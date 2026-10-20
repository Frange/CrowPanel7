#include "screen_list.h"
#include "../ui_manager.h"

static void refresh_btn_event_cb(lv_event_t * e) {
    (void)e;
}

static void add_btn_event_cb(lv_event_t * e) {
    (void)e;
    ui_change_screen(SCREEN_ADD);
}

static void settings_btn_event_cb(lv_event_t * e) {
    (void)e;
    ui_change_screen(SCREEN_SETTINGS);
}

lv_obj_t* screen_list_create() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121212), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ============================================================
    // TOP TOOLBAR
    // ============================================================
    lv_obj_t *top_bar = lv_obj_create(scr);
    lv_obj_set_size(top_bar, 800, 65);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1F1F1F), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 10, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Mi Portafolio (Acciones / ETFs)");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Boton Ajustes
    lv_obj_t *btn_settings = lv_btn_create(top_bar);
    lv_obj_set_size(btn_settings, 48, 48);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(btn_settings, 12, 0);
    lv_obj_add_event_cb(btn_settings, settings_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_settings = lv_label_create(btn_settings);
    lv_label_set_text(lbl_settings, LV_SYMBOL_SETTINGS);
    lv_obj_center(lbl_settings);

    // Boton Anadir (+)
    lv_obj_t *btn_add = lv_btn_create(top_bar);
    lv_obj_set_size(btn_add, 48, 48);
    lv_obj_align(btn_add, LV_ALIGN_RIGHT_MID, -68, 0);
    lv_obj_set_style_bg_color(btn_add, lv_color_hex(0x007ACC), 0);
    lv_obj_set_style_radius(btn_add, 12, 0);
    lv_obj_add_event_cb(btn_add, add_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_add = lv_label_create(btn_add);
    lv_label_set_text(lbl_add, LV_SYMBOL_PLUS);
    lv_obj_center(lbl_add);

    // Boton Actualizar
    lv_obj_t *btn_refresh = lv_btn_create(top_bar);
    lv_obj_set_size(btn_refresh, 48, 48);
    lv_obj_align(btn_refresh, LV_ALIGN_RIGHT_MID, -126, 0);
    lv_obj_set_style_bg_color(btn_refresh, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(btn_refresh, 12, 0);
    lv_obj_add_event_cb(btn_refresh, refresh_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_refresh = lv_label_create(btn_refresh);
    lv_label_set_text(lbl_refresh, LV_SYMBOL_REFRESH);
    lv_obj_center(lbl_refresh);

    // ============================================================
    // CONTENEDOR PRINCIPAL / ESTADO VACIO
    // ============================================================
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 800, 415);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x121212), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 15, 0);

    bool has_items = false;

    if (!has_items) {
        lv_obj_t *empty_cnt = lv_obj_create(content);
        lv_obj_set_size(empty_cnt, 420, 200);
        lv_obj_center(empty_cnt);
        lv_obj_set_style_bg_color(empty_cnt, lv_color_hex(0x1E1E1E), 0);
        lv_obj_set_style_border_width(empty_cnt, 1, 0);
        lv_obj_set_style_border_color(empty_cnt, lv_color_hex(0x333333), 0);
        lv_obj_set_style_radius(empty_cnt, 16, 0);
        lv_obj_clear_flag(empty_cnt, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *msg = lv_label_create(empty_cnt);
        lv_label_set_text(msg, "No hay acciones o ETFs anadidos");
        lv_obj_set_style_text_color(msg, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 30);

        lv_obj_t *btn_center_add = lv_btn_create(empty_cnt);
        lv_obj_set_size(btn_center_add, 200, 50);
        lv_obj_align(btn_center_add, LV_ALIGN_BOTTOM_MID, 0, -25);
        lv_obj_set_style_bg_color(btn_center_add, lv_color_hex(0x007ACC), 0);
        lv_obj_set_style_radius(btn_center_add, 10, 0);
        lv_obj_add_event_cb(btn_center_add, add_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl_btn = lv_label_create(btn_center_add);
        lv_label_set_text(lbl_btn, LV_SYMBOL_PLUS " Anadir Ticker");
        lv_obj_center(lbl_btn);
    }

    return scr;
}