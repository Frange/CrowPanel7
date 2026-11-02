
#include "screen_add.h"

#include <Arduino.h>
#include <WiFi.h>

#include "../../ui_manager.h"
#include "../../ui_keyboard.h"

#include "../../../services/storage_service.h"
#include "../../../services/market_service.h"

static lv_obj_t *add_status_label = nullptr;
static lv_obj_t *save_button = nullptr;
static lv_timer_t *validation_timer = nullptr;
static String pending_ticker;
static bool validation_in_progress = false;

static void set_status(const char *message, uint32_t color)
{
    if (add_status_label == nullptr) {
        return;
    }

    lv_label_set_text(add_status_label, message);
    lv_obj_set_style_text_color(
        add_status_label,
        lv_color_hex(color),
        0
    );
}

static void stop_validation_timer()
{
    if (validation_timer != nullptr) {
        lv_timer_del(validation_timer);
        validation_timer = nullptr;
    }

    validation_in_progress = false;
}

static void screen_delete_cb(lv_event_t *e)
{
    (void)e;

    stop_validation_timer();
    add_status_label = nullptr;
    pending_ticker = "";
}

static void back_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_LIST);
}

// La tecla OK del teclado valida igual que el boton.
static void keyboard_ready_cb(lv_event_t *e)
{
    lv_obj_t *keyboard = lv_event_get_target(e);
    lv_obj_t *ta = lv_keyboard_get_textarea(keyboard);

    if (ta == nullptr || save_button == nullptr) {
        return;
    }

    lv_event_send(save_button, LV_EVENT_CLICKED, nullptr);
}

static void validation_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    String symbol;
    StockData data;
    bool success = false;

    if (!market_service_take_validation_result(
            symbol, data, success)) {
        return;
    }

    stop_validation_timer();

    if (symbol != pending_ticker) {
        set_status(
            "La respuesta no corresponde al ticker solicitado.",
            0xFF5555
        );
        return;
    }

    if (!success || !data.valid) {
        set_status(
            "Ticker no validado. Comprueba el simbolo o la conexion.",
            0xFF5555
        );
        return;
    }

    if (storage_ticker_exists(pending_ticker)) {
        set_status(
            "Ese ticker ya esta guardado.",
            0xFFCC66
        );
        return;
    }

    if (!storage_add_ticker(pending_ticker)) {
        set_status(
            "No se pudo guardar el ticker.",
            0xFF5555
        );
        return;
    }

    Serial.printf(
        "[ADD] %s validado. Precio: %.4f %s\n",
        pending_ticker.c_str(),
        data.price,
        data.currency.c_str()
    );

    pending_ticker = "";

    ui_change_screen(SCREEN_LIST);
}

static void save_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *ta =
        static_cast<lv_obj_t *>(lv_event_get_user_data(e));

    if (ta == nullptr || add_status_label == nullptr) {
        return;
    }

    if (validation_in_progress) {
        return;
    }

    String ticker =
        storage_normalize_ticker(lv_textarea_get_text(ta));

    if (!storage_ticker_format_valid(ticker)) {
        set_status(
            "Simbolo no valido. Ejemplos: AAPL, SPY o ZAL.DE",
            0xFF5555
        );
        return;
    }

    if (storage_ticker_exists(ticker)) {
        set_status("Ese ticker ya esta guardado.", 0xFFCC66);
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        set_status(
            "Conecta el WiFi antes de validar el ticker.",
            0xFF5555
        );
        return;
    }

    pending_ticker = ticker;

    set_status("Consultando Yahoo Finance...", 0xCCCCCC);

    if (!market_service_validate_async(pending_ticker)) {
        set_status(
            "Servicio ocupado. Intentalo de nuevo.",
            0xFFCC66
        );
        pending_ticker = "";
        return;
    }

    validation_in_progress = true;

    validation_timer = lv_timer_create(
        validation_timer_cb,
        250,
        nullptr
    );
}

lv_obj_t *screen_add_create()
{
    stop_validation_timer();
    pending_ticker = "";

    lv_obj_t *scr = lv_obj_create(nullptr);
    add_status_label = nullptr;
    save_button = nullptr;

    lv_obj_add_event_cb(scr, screen_delete_cb, LV_EVENT_DELETE, nullptr);

    lv_obj_set_style_bg_color(
        scr, lv_color_hex(0x121212), 0
    );
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *top_bar = lv_obj_create(scr);
    lv_obj_set_size(top_bar, 800, 65);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_set_style_bg_color(
        top_bar, lv_color_hex(0x1F1F1F), 0
    );
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(top_bar);
    lv_obj_set_size(btn_back, 90, 42);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 15, 0);

    lv_obj_set_style_bg_color(
        btn_back, lv_color_hex(0x333333), 0
    );
    lv_obj_set_style_radius(btn_back, 8, 0);

    lv_obj_add_event_cb(
        btn_back,
        back_btn_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Volver");
    lv_obj_center(lbl_back);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Anadir Accion / ETF");

    lv_obj_set_style_text_color(
        title, lv_color_hex(0xFFFFFF), 0
    );
    lv_obj_set_style_text_font(
        title, &lv_font_montserrat_14, 0
    );
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 720, 235);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 82);

    lv_obj_set_style_bg_color(
        content, lv_color_hex(0x1E1E1E), 0
    );
    lv_obj_set_style_border_width(content, 1, 0);
    lv_obj_set_style_border_color(
        content, lv_color_hex(0x333333), 0
    );
    lv_obj_set_style_radius(content, 16, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_info = lv_label_create(content);
    lv_label_set_text(
        lbl_info,
        "Introduce un simbolo reconocido por Yahoo Finance."
    );

    lv_obj_set_style_text_color(
        lbl_info, lv_color_hex(0xCCCCCC), 0
    );
    lv_obj_align(lbl_info, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *ta = lv_textarea_create(content);
    lv_obj_set_size(ta, 450, 45);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 45);

    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 16);

    lv_textarea_set_accepted_chars(
        ta,
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789.-^="
    );

    lv_textarea_set_placeholder_text(
        ta, "Ej: AAPL, SPY, ZAL.DE"
    );

    lv_obj_t *btn_save = lv_btn_create(content);
    save_button = btn_save;
    lv_obj_set_size(btn_save, 180, 42);
    lv_obj_align(btn_save, LV_ALIGN_TOP_MID, 0, 102);

    lv_obj_set_style_bg_color(
        btn_save, lv_color_hex(0x007ACC), 0
    );
    lv_obj_set_style_radius(btn_save, 8, 0);

    lv_obj_add_event_cb(
        btn_save,
        save_btn_event_cb,
        LV_EVENT_CLICKED,
        ta
    );

    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "Validar y guardar");
    lv_obj_center(lbl_save);

    add_status_label = lv_label_create(content);
    lv_obj_set_width(add_status_label, 680);
    lv_label_set_long_mode(
        add_status_label, LV_LABEL_LONG_WRAP
    );

    lv_obj_set_style_text_color(
        add_status_label, lv_color_hex(0xAAAAAA), 0
    );
    lv_obj_set_style_text_align(
        add_status_label, LV_TEXT_ALIGN_CENTER, 0
    );
    lv_obj_align(
        add_status_label, LV_ALIGN_TOP_MID, 0, 152
    );

    lv_label_set_text(add_status_label, " ");

    lv_obj_t *keyboard = ui_keyboard_attach(
        scr, ta, LV_KEYBOARD_MODE_TEXT_UPPER
    );

    if (keyboard != nullptr) {
        lv_obj_add_event_cb(
            keyboard, keyboard_ready_cb, LV_EVENT_READY, nullptr
        );
    }

    // Mostrar el teclado directamente: es lo unico que se hace aqui.
    lv_obj_add_state(ta, LV_STATE_FOCUSED);
    lv_event_send(ta, LV_EVENT_FOCUSED, nullptr);

    return scr;
}
