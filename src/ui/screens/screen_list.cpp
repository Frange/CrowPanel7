#include "screen_list.h"
#include "../ui_manager.h"
#include "../../services/storage_service.h"

static lv_obj_t *wifi_indicator = nullptr;

static void refresh_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_LIST);
}

static void add_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_ADD);
}

static void settings_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_SETTINGS);
}

static void update_wifi_indicator()
{
    if (wifi_indicator == nullptr) return;

    int status = storage_wifi_status();

    switch (status) {
        case STORAGE_WIFI_INTERNET:
            lv_label_set_text(wifi_indicator, LV_SYMBOL_WIFI " Internet");
            lv_obj_set_style_text_color(
                wifi_indicator, lv_color_hex(0x55DD88), 0
            );
            break;

        case STORAGE_WIFI_NO_INTERNET:
            lv_label_set_text(wifi_indicator, LV_SYMBOL_WIFI " Sin Internet");
            lv_obj_set_style_text_color(
                wifi_indicator, lv_color_hex(0xFFCC66), 0
            );
            break;

        default:
            lv_label_set_text(wifi_indicator, LV_SYMBOL_WIFI " Sin WiFi");
            lv_obj_set_style_text_color(
                wifi_indicator, lv_color_hex(0xAAAAAA), 0
            );
            break;
    }
}

lv_obj_t *screen_list_create()
{
    lv_obj_t *scr = lv_obj_create(nullptr);
    wifi_indicator = nullptr;

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121212), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

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

    wifi_indicator = lv_label_create(top_bar);
    lv_obj_set_width(wifi_indicator, 145);
    lv_label_set_long_mode(wifi_indicator, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(wifi_indicator, &lv_font_montserrat_14, 0);
    lv_obj_align(wifi_indicator, LV_ALIGN_RIGHT_MID, -180, 0);
    lv_label_set_text(wifi_indicator, LV_SYMBOL_WIFI " ...");

    lv_obj_t *btn_settings = lv_btn_create(top_bar);
    lv_obj_set_size(btn_settings, 48, 48);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(btn_settings, 12, 0);
    lv_obj_add_event_cb(
        btn_settings, settings_btn_event_cb, LV_EVENT_CLICKED, nullptr
    );

    lv_obj_t *lbl_settings = lv_label_create(btn_settings);
    lv_label_set_text(lbl_settings, LV_SYMBOL_SETTINGS);
    lv_obj_center(lbl_settings);

    lv_obj_t *btn_add = lv_btn_create(top_bar);
    lv_obj_set_size(btn_add, 48, 48);
    lv_obj_align(btn_add, LV_ALIGN_RIGHT_MID, -68, 0);
    lv_obj_set_style_bg_color(btn_add, lv_color_hex(0x007ACC), 0);
    lv_obj_set_style_radius(btn_add, 12, 0);
    lv_obj_add_event_cb(
        btn_add, add_btn_event_cb, LV_EVENT_CLICKED, nullptr
    );

    lv_obj_t *lbl_add = lv_label_create(btn_add);
    lv_label_set_text(lbl_add, LV_SYMBOL_PLUS);
    lv_obj_center(lbl_add);

    lv_obj_t *btn_refresh = lv_btn_create(top_bar);
    lv_obj_set_size(btn_refresh, 48, 48);
    lv_obj_align(btn_refresh, LV_ALIGN_RIGHT_MID, -126, 0);
    lv_obj_set_style_bg_color(btn_refresh, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(btn_refresh, 12, 0);
    lv_obj_add_event_cb(
        btn_refresh, refresh_btn_event_cb, LV_EVENT_CLICKED, nullptr
    );

    lv_obj_t *lbl_refresh = lv_label_create(btn_refresh);
    lv_label_set_text(lbl_refresh, LV_SYMBOL_REFRESH);
    lv_obj_center(lbl_refresh);

    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 800, 415);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x121212), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 15, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    size_t count = storage_ticker_count();

    if (count == 0) {
        lv_obj_t *empty_cnt = lv_obj_create(content);
        lv_obj_set_size(empty_cnt, 420, 200);
        lv_obj_center(empty_cnt);
        lv_obj_set_style_bg_color(
            empty_cnt, lv_color_hex(0x1E1E1E), 0
        );
        lv_obj_set_style_border_width(empty_cnt, 1, 0);
        lv_obj_set_style_border_color(
            empty_cnt, lv_color_hex(0x333333), 0
        );
        lv_obj_set_style_radius(empty_cnt, 16, 0);
        lv_obj_clear_flag(empty_cnt, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *msg = lv_label_create(empty_cnt);
        lv_label_set_text(msg, "No hay acciones o ETFs anadidos");
        lv_obj_set_style_text_color(
            msg, lv_color_hex(0xAAAAAA), 0
        );
        lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 30);

        lv_obj_t *btn_center_add = lv_btn_create(empty_cnt);
        lv_obj_set_size(btn_center_add, 200, 50);
        lv_obj_align(btn_center_add, LV_ALIGN_BOTTOM_MID, 0, -25);
        lv_obj_set_style_bg_color(
            btn_center_add, lv_color_hex(0x007ACC), 0
        );
        lv_obj_set_style_radius(btn_center_add, 10, 0);
        lv_obj_add_event_cb(
            btn_center_add, add_btn_event_cb, LV_EVENT_CLICKED, nullptr
        );

        lv_obj_t *lbl_btn = lv_label_create(btn_center_add);
        lv_label_set_text(lbl_btn, LV_SYMBOL_PLUS " Anadir Ticker");
        lv_obj_center(lbl_btn);
    } else {
        lv_obj_t *ticker_list = lv_list_create(content);
        lv_obj_set_size(ticker_list, 760, 380);
        lv_obj_align(ticker_list, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(
            ticker_list, lv_color_hex(0x181818), 0
        );
        lv_obj_set_style_border_width(ticker_list, 0, 0);

        for (size_t i = 0; i < count; ++i) {
            String ticker = storage_get_ticker(i);
            if (ticker.length() == 0) continue;

            String row = ticker + "    Guardado";
            lv_obj_t *btn = lv_list_add_btn(
                ticker_list, LV_SYMBOL_OK, row.c_str()
            );
            (void)btn;
        }
    }

    // Prueba de conectividad al entrar en la pantalla y al pulsar Actualizar.
    update_wifi_indicator();

    return scr;
}
