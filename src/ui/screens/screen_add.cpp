#include "screen_add.h"
#include "../ui_manager.h"

static void back_btn_event_cb(lv_event_t * e) {
    ui_change_screen(SCREEN_LIST);
}

lv_obj_t* screen_add_create() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121212), 0);

    lv_obj_t *btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, 100, 45);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_add_event_cb(btn_back, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Cancelar");
    lv_obj_center(lbl_back);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Pantalla de Añadir Ticker");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(title);

    return scr;
}