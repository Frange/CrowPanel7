#include "screen_add.h"
#include "../ui_manager.h"
#include "../ui_keyboard.h"
#include "../../services/storage_service.h"

static lv_obj_t *add_status_label = nullptr;

static bool ticker_format_valid(String ticker)
{
    ticker.trim();
    ticker.toUpperCase();

    Serial.printf("[DEBUG] Validando ticker: '%s' (longitud: %u)\n", ticker.c_str(), (unsigned int)ticker.length());

    if (ticker.length() == 0 || ticker.length() > 16) {
        Serial.println("[DEBUG] Falla por longitud.");
        return false;
    }

    bool has_alphanumeric = false;

    for (size_t i = 0; i < ticker.length(); ++i) {
        char c = ticker.charAt(i);
        bool alpha = c >= 'A' && c <= 'Z';
        bool digit = c >= '0' && c <= '9';

        if (alpha || digit) {
            has_alphanumeric = true;
        } else if (c != '.' && c != '-' && c != '^' && c != '=') {
            Serial.printf("[DEBUG] Caracter no permitido encontrado: '%c' (ASCII: %d)\n", c, (int)c);
            return false;
        }
    }

    if (!has_alphanumeric) {
        Serial.println("[DEBUG] Falla por falta de caracteres alfanumericos.");
        return false;
    }

    return true;
}

static void back_btn_event_cb(lv_event_t *e)
{
    (void)e;
    add_status_label = nullptr;
    ui_change_screen(SCREEN_LIST);
}

static void save_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
    if (ta == nullptr || add_status_label == nullptr) return;

    String ticker = lv_textarea_get_text(ta);
    ticker.trim();
    ticker.toUpperCase();

    Serial.printf("[SAVE] Intentando guardar ticker bruto: '%s'\n", ticker.c_str());

    if (!ticker_format_valid(ticker)) {
        lv_label_set_text(
            add_status_label,
            "Error: Simbolo no valido. Usa letras, numeros o punto (.)"
        );
        lv_obj_set_style_text_color(
            add_status_label, lv_color_hex(0xFF5555), 0
        );
        return;
    }

    if (storage_ticker_exists(ticker)) {
        lv_label_set_text(add_status_label, "Aviso: Ese ticker ya esta guardado.");
        lv_obj_set_style_text_color(
            add_status_label, lv_color_hex(0xFFCC66), 0
        );
        return;
    }

    if (!storage_add_ticker(ticker)) {
        lv_label_set_text(
            add_status_label,
            "Error al guardar en memoria. Reintentalo."
        );
        lv_obj_set_style_text_color(
            add_status_label, lv_color_hex(0xFF5555), 0
        );
        return;
    }

    lv_label_set_text(add_status_label, "¡Ticker guardado con exito!");
    lv_obj_set_style_text_color(add_status_label, lv_color_hex(0x55DD88), 0);

    add_status_label = nullptr;
    ui_change_screen(SCREEN_LIST);
}

lv_obj_t *screen_add_create()
{
    lv_obj_t *scr = lv_obj_create(nullptr);
    add_status_label = nullptr;

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121212), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

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
    lv_obj_add_event_cb(
        btn_back, back_btn_event_cb, LV_EVENT_CLICKED, nullptr
    );

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Volver");
    lv_obj_center(lbl_back);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Anadir Nueva Accion / ETF");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 720, 205);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 72);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_width(content, 1, 0);
    lv_obj_set_style_border_color(content, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(content, 16, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_info = lv_label_create(content);
    lv_label_set_text(
        lbl_info,
        "Introduce el simbolo del ticker (ej. AAPL, TSLA, ZAL.DE):"
    );
    lv_obj_set_style_text_color(lbl_info, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl_info, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *ta = lv_textarea_create(content);
    lv_obj_set_size(ta, 450, 45);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 38);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 16);
    lv_textarea_set_accepted_chars(
        ta, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-^="
    );
    lv_textarea_set_placeholder_text(ta, "Ej: ZAL.DE");

    lv_obj_t *btn_save = lv_btn_create(content);
    lv_obj_set_size(btn_save, 180, 40);
    lv_obj_align(btn_save, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x007ACC), 0);
    lv_obj_set_style_radius(btn_save, 8, 0);
    lv_obj_add_event_cb(
        btn_save, save_btn_event_cb, LV_EVENT_CLICKED, ta
    );

    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "Guardar");
    lv_obj_center(lbl_save);

    add_status_label = lv_label_create(content);
    lv_obj_set_width(add_status_label, 680);
    lv_label_set_long_mode(add_status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(
        add_status_label, lv_color_hex(0xAAAAAA), 0
    );
    lv_obj_align(add_status_label, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_label_set_text(add_status_label, " ");

    ui_keyboard_attach(scr, ta, LV_KEYBOARD_MODE_TEXT_UPPER);

    return scr;
}