#include "screen_settings.h"
#include "../ui_manager.h"

static void back_btn_event_cb(lv_event_t * e) {
    (void)e;
    ui_change_screen(SCREEN_LIST);
}

static void scan_wifi_event_cb(lv_event_t * e) {
    (void)e;
    // Lógica para buscar redes WiFi
}

lv_obj_t* screen_settings_create() {
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
    lv_label_set_text(title, "Configuracion del Sistema y Red WiFi");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Contenido Principal (Sección WiFi)
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 740, 380);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_width(content, 1, 0);
    lv_obj_set_style_border_color(content, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(content, 16, 0);

    // Título de sección WiFi
    lv_obj_t *lbl_wifi_title = lv_label_create(content);
    lv_label_set_text(lbl_wifi_title, "Redes WiFi Disponibles:");
    lv_obj_set_style_text_color(lbl_wifi_title, lv_color_hex(0x007ACC), 0);
    lv_obj_align(lbl_wifi_title, LV_ALIGN_TOP_LEFT, 15, 15);

    // Botón escanear redes
    lv_obj_t *btn_scan = lv_btn_create(content);
    lv_obj_set_size(btn_scan, 150, 38);
    lv_obj_align(btn_scan, LV_ALIGN_TOP_RIGHT, -15, 10);
    lv_obj_set_style_bg_color(btn_scan, lv_color_hex(0x007ACC), 0);
    lv_obj_set_style_radius(btn_scan, 8, 0);
    lv_obj_add_event_cb(btn_scan, scan_wifi_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_scan = lv_label_create(btn_scan);
    lv_label_set_text(lbl_scan, LV_SYMBOL_REFRESH " Buscar");
    lv_obj_center(lbl_scan);

    // Lista scrolleable de redes WiFi detectadas
    lv_obj_t *wifi_list = lv_list_create(content);
    lv_obj_set_size(wifi_list, 705, 280);
    lv_obj_align(wifi_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x181818), 0);
    lv_obj_set_style_border_width(wifi_list, 0, 0);

    // Elementos simulados de redes WiFi encontrados al escanear
    lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, "MiRed_Casa_5G (Conectado)");
    lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, "Movistar_FIBRA_1234");
    lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, "Vodafone_WiFi_Ext");
    lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, "Orange_Guest");

    return scr;
}