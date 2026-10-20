#include "screen_settings.h"
#include "../ui_manager.h"
#include "../ui_keyboard.h"
#include "../../services/storage_service.h"

#include <WiFi.h>

static lv_obj_t *wifi_list = nullptr;
static lv_obj_t *wifi_title = nullptr;
static lv_obj_t *scan_button = nullptr;
static lv_obj_t *password_panel = nullptr;
static lv_obj_t *password_textarea = nullptr;
static lv_obj_t *wifi_status_label = nullptr;
static lv_obj_t *keyboard = nullptr;
static lv_timer_t *connect_timer = nullptr;

static String scanned_ssids[30];
static size_t scanned_count = 0;
static String selected_ssid;
static String pending_password;
static uint32_t connect_started_ms = 0;

static void wifi_connect_timer_cb(lv_timer_t *timer);

static void stop_connect_timer()
{
    if (connect_timer != nullptr) {
        lv_timer_del(connect_timer);
        connect_timer = nullptr;
    }
}

static void back_btn_event_cb(lv_event_t *e)
{
    (void)e;
    stop_connect_timer();
    wifi_list = nullptr;
    password_panel = nullptr;
    password_textarea = nullptr;
    wifi_status_label = nullptr;
    keyboard = nullptr;
    ui_change_screen(SCREEN_LIST);
}

static void show_network_list()
{
    if (password_panel) lv_obj_add_flag(password_panel, LV_OBJ_FLAG_HIDDEN);
    if (wifi_list) lv_obj_clear_flag(wifi_list, LV_OBJ_FLAG_HIDDEN);
    if (wifi_title) lv_obj_clear_flag(wifi_title, LV_OBJ_FLAG_HIDDEN);
    if (scan_button) lv_obj_clear_flag(scan_button, LV_OBJ_FLAG_HIDDEN);
}

static void select_network_event_cb(lv_event_t *e)
{
    String *ssid = (String *)lv_event_get_user_data(e);
    if (ssid == nullptr || ssid->length() == 0) return;

    selected_ssid = *ssid;

    lv_obj_add_flag(wifi_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifi_title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(scan_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(password_panel, LV_OBJ_FLAG_HIDDEN);

    lv_textarea_set_text(password_textarea, "");
    lv_label_set_text_fmt(wifi_status_label, "Red seleccionada: %s",
                          selected_ssid.c_str());
}

static void scan_wifi_event_cb(lv_event_t *e)
{
    (void)e;

    stop_connect_timer();
    lv_label_set_text(wifi_status_label, "Buscando redes WiFi...");
    lv_refr_now(nullptr);

    lv_obj_clean(wifi_list);
    scanned_count = 0;

    WiFi.mode(WIFI_STA);
    WiFi.scanDelete();

    int count = WiFi.scanNetworks(false, true);

    if (count < 0) {
        lv_label_set_text(wifi_status_label, "No se pudo completar el escaneo.");
        return;
    }

    for (int i = 0; i < count && scanned_count < 30; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;

        scanned_ssids[scanned_count] = ssid;

        String label = ssid;
        label += "  (";
        label += String(WiFi.RSSI(i));
        label += " dBm)";

        if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
            label += "  [abierta]";
        }

        lv_obj_t *btn = lv_list_add_btn(
            wifi_list, LV_SYMBOL_WIFI, label.c_str()
        );

        lv_obj_add_event_cb(
            btn, select_network_event_cb, LV_EVENT_CLICKED,
            &scanned_ssids[scanned_count]
        );

        ++scanned_count;
    }

    WiFi.scanDelete();

    if (scanned_count == 0) {
        lv_label_set_text(
            wifi_status_label,
            "No se encontraron redes. Pulsa Buscar para reintentar."
        );
    } else {
        lv_label_set_text_fmt(
            wifi_status_label, "Redes encontradas: %u. Pulsa una para conectarte.",
            (unsigned int)scanned_count
        );
    }
}

static void show_networks_event_cb(lv_event_t *e)
{
    (void)e;
    show_network_list();
}

static void wifi_connect_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (WiFi.status() == WL_CONNECTED) {
        storage_save_wifi_credentials(selected_ssid, pending_password);
        lv_label_set_text_fmt(
            wifi_status_label, "Conectado a %s. Credenciales guardadas.",
            selected_ssid.c_str()
        );
        stop_connect_timer();
        return;
    }

    if ((uint32_t)(millis() - connect_started_ms) > 20000U) {
        lv_label_set_text(
            wifi_status_label,
            "No se pudo conectar. Comprueba la contrasena e intentalo otra vez."
        );
        WiFi.disconnect(false, false);
        stop_connect_timer();
    }
}

static void connect_wifi_event_cb(lv_event_t *e)
{
    (void)e;

    if (selected_ssid.length() == 0) {
        lv_label_set_text(wifi_status_label, "Selecciona primero una red.");
        return;
    }

    pending_password = lv_textarea_get_text(password_textarea);

    if (keyboard != nullptr) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(wifi_status_label, "Conectando... espera hasta 20 segundos.");
    lv_refr_now(nullptr);

    stop_connect_timer();

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.disconnect(false, false);
    WiFi.begin(selected_ssid.c_str(), pending_password.c_str());

    connect_started_ms = millis();
    connect_timer = lv_timer_create(wifi_connect_timer_cb, 400, nullptr);
}

lv_obj_t *screen_settings_create()
{
    lv_obj_t *scr = lv_obj_create(nullptr);

    wifi_list = nullptr;
    wifi_title = nullptr;
    scan_button = nullptr;
    password_panel = nullptr;
    password_textarea = nullptr;
    wifi_status_label = nullptr;
    keyboard = nullptr;
    stop_connect_timer();

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
    lv_label_set_text(title, "Configuracion WiFi");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    wifi_status_label = lv_label_create(scr);
    lv_obj_set_width(wifi_status_label, 590);
    lv_label_set_long_mode(wifi_status_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(
        wifi_status_label, lv_color_hex(0xCCCCCC), 0
    );
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_LEFT, 20, 76);
    lv_label_set_text(wifi_status_label, "Preparando escaneo...");

    wifi_title = lv_label_create(scr);
    lv_label_set_text(wifi_title, "Redes WiFi disponibles");
    lv_obj_set_style_text_color(wifi_title, lv_color_hex(0x007ACC), 0);
    lv_obj_align(wifi_title, LV_ALIGN_TOP_LEFT, 20, 102);

    scan_button = lv_btn_create(scr);
    lv_obj_set_size(scan_button, 145, 38);
    lv_obj_align(scan_button, LV_ALIGN_TOP_RIGHT, -20, 94);
    lv_obj_set_style_bg_color(scan_button, lv_color_hex(0x007ACC), 0);
    lv_obj_add_event_cb(
        scan_button, scan_wifi_event_cb, LV_EVENT_CLICKED, nullptr
    );

    lv_obj_t *lbl_scan = lv_label_create(scan_button);
    lv_label_set_text(lbl_scan, LV_SYMBOL_REFRESH " Buscar");
    lv_obj_center(lbl_scan);

    wifi_list = lv_list_create(scr);
    lv_obj_set_size(wifi_list, 760, 350);
    lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, 125);
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x181818), 0);
    lv_obj_set_style_border_width(wifi_list, 0, 0);

    password_panel = lv_obj_create(scr);
    lv_obj_set_size(password_panel, 740, 180);
    lv_obj_align(password_panel, LV_ALIGN_TOP_MID, 0, 88);
    lv_obj_set_style_bg_color(password_panel, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(password_panel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(password_panel, 12, 0);
    lv_obj_clear_flag(password_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(password_panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lbl_password_title = lv_label_create(password_panel);
    lv_label_set_text(lbl_password_title, "Conectar a la red seleccionada");
    lv_obj_set_style_text_color(
        lbl_password_title, lv_color_hex(0xFFFFFF), 0
    );
    lv_obj_align(lbl_password_title, LV_ALIGN_TOP_MID, 0, 8);

    password_textarea = lv_textarea_create(password_panel);
    lv_obj_set_size(password_textarea, 400, 45);
    lv_obj_align(password_textarea, LV_ALIGN_TOP_MID, 0, 38);
    lv_textarea_set_one_line(password_textarea, true);
    lv_textarea_set_password_mode(password_textarea, true);
    lv_textarea_set_max_length(password_textarea, 63);
    lv_textarea_set_placeholder_text(password_textarea, "Contrasena WiFi");

    lv_obj_t *btn_connect = lv_btn_create(password_panel);
    lv_obj_set_size(btn_connect, 150, 38);
    lv_obj_align(btn_connect, LV_ALIGN_BOTTOM_MID, 85, -8);
    lv_obj_set_style_bg_color(btn_connect, lv_color_hex(0x007ACC), 0);
    lv_obj_add_event_cb(
        btn_connect, connect_wifi_event_cb, LV_EVENT_CLICKED, nullptr
    );

    lv_obj_t *lbl_connect = lv_label_create(btn_connect);
    lv_label_set_text(lbl_connect, "Conectar");
    lv_obj_center(lbl_connect);

    lv_obj_t *btn_networks = lv_btn_create(password_panel);
    lv_obj_set_size(btn_networks, 150, 38);
    lv_obj_align(btn_networks, LV_ALIGN_BOTTOM_MID, -85, -8);
    lv_obj_set_style_bg_color(btn_networks, lv_color_hex(0x333333), 0);
    lv_obj_add_event_cb(
        btn_networks, show_networks_event_cb, LV_EVENT_CLICKED, nullptr
    );

    lv_obj_t *lbl_networks = lv_label_create(btn_networks);
    lv_label_set_text(lbl_networks, "Otras redes");
    lv_obj_center(lbl_networks);

    keyboard = ui_keyboard_attach(
        scr, password_textarea, LV_KEYBOARD_MODE_TEXT_LOWER
    );

    // El escaneo es real. La lista se llena con los resultados del ESP32.
    scan_wifi_event_cb(nullptr);

    return scr;
}
