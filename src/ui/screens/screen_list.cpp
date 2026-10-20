

#include <Arduino.h>
#include <WiFi.h>
#include "screen_detail.h"
#include "screen_list.h"
#include "../ui_manager.h"

#include "../../services/storage_service.h"
#include "../../services/market_service.h"

namespace {

constexpr size_t MAX_ROWS = 40;
constexpr uint32_t AUTO_REFRESH_MS = 300000;
constexpr uint32_t UI_POLL_MS = 1000;

lv_obj_t *list_screen = nullptr;
lv_obj_t *wifi_indicator = nullptr;
lv_obj_t *refresh_status = nullptr;

lv_obj_t *ticker_labels[MAX_ROWS] = {};
lv_obj_t *price_labels[MAX_ROWS] = {};
lv_obj_t *open_labels[MAX_ROWS] = {};
lv_obj_t *close_labels[MAX_ROWS] = {};
lv_obj_t *change_labels[MAX_ROWS] = {};
lv_obj_t *delete_buttons[MAX_ROWS] = {};

lv_obj_t *list_content = nullptr;

lv_timer_t *list_timer = nullptr;

uint32_t last_refresh_request = 0;
bool first_refresh_requested = false;
void ticker_row_event_cb(lv_event_t *e);

void update_rows();

void ticker_row_event_cb(lv_event_t *e)
{
    size_t index = static_cast<size_t>(
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(e)
        )
    );

    if (index >= MAX_ROWS) {
        return;
    }

    String symbol = storage_get_ticker(index);

    if (symbol.isEmpty()) {
        return;
    }

    screen_detail_set_symbol(symbol);
    ui_change_screen(SCREEN_DETAIL);
}

void refresh_btn_event_cb(lv_event_t *e)
{
    (void)e;

    if (market_service_request_refresh()) {
        last_refresh_request = millis();

        if (refresh_status != nullptr) {
            lv_label_set_text(
                refresh_status,
                "Actualizando cotizaciones..."
            );
        }
    }
}

void add_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_ADD);
}

void settings_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_SETTINGS);
}


void delete_btn_event_cb(lv_event_t *e)
{
    size_t index = static_cast<size_t>(
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(e)
        )
    );

    if (index >= MAX_ROWS) {
        return;
    }

    String symbol = storage_get_ticker(index);

    if (symbol.isEmpty()) {
        return;
    }

    // Borrar el ticker del almacenamiento persistente.
    if (!storage_remove_ticker(index)) {
        Serial.printf(
            "[UI] Error al eliminar el ticker %s\n",
            symbol.c_str()
        );

        return;
    }

    Serial.printf(
        "[UI] Ticker %s eliminado correctamente\n",
        symbol.c_str()
    );

    // Ocultar las filas que ya no corresponden a tickers.
    size_t count = storage_ticker_count();

    for (size_t i = index; i < MAX_ROWS; ++i) {
        if (ticker_labels[i] == nullptr) {
            continue;
        }

        lv_obj_t *row = lv_obj_get_parent(ticker_labels[i]);

        if (i < count) {
            // Actualizar el símbolo para que coincida
            // con su nuevo índice en el almacenamiento.
            String remaining_symbol = storage_get_ticker(i);

            lv_label_set_text(
                ticker_labels[i],
                remaining_symbol.c_str()
            );

            lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);

            // Recolocar la fila en su nueva posición.
            lv_obj_align(
                row,
                LV_ALIGN_TOP_LEFT,
                0,
                static_cast<lv_coord_t>(i * 53)
            );
        } else {
            // Ya no existe ningún ticker en esta posición.
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Actualizar precios, apertura, cierre y porcentaje.
    update_rows();

    // Mostrar el mensaje de lista vacía si ya no quedan tickers.
    if (count == 0 && list_content != nullptr) {
        lv_obj_t *empty = lv_label_create(list_content);

        lv_label_set_text(
            empty,
            "No hay acciones ni ETFs.\n"
            "Pulsa + para anadir tu primer ticker."
        );

        lv_obj_set_style_text_color(
            empty,
            lv_color_hex(0xAAAAAA),
            0
        );

        lv_obj_align(
            empty,
            LV_ALIGN_TOP_MID,
            0,
            40
        );
    }
}

void update_wifi_indicator()
{
    if (wifi_indicator == nullptr) {
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text(
            wifi_indicator,
            LV_SYMBOL_WIFI " Conectado"
        );

        lv_obj_set_style_text_color(
            wifi_indicator,
            lv_color_hex(0x55DD88),
            0
        );
    } else {
        lv_label_set_text(
            wifi_indicator,
            LV_SYMBOL_WIFI " Sin WiFi"
        );

        lv_obj_set_style_text_color(
            wifi_indicator,
            lv_color_hex(0xFFCC66),
            0
        );
    }
}

void update_rows()
{
    size_t count = storage_ticker_count();

    if (count > MAX_ROWS) {
        count = MAX_ROWS;
    }

    for (size_t i = 0; i < MAX_ROWS; ++i) {
        if (ticker_labels[i] == nullptr) {
            continue;
        }

        lv_obj_t *row =
            lv_obj_get_parent(ticker_labels[i]);

        if (i >= count) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);

        String symbol = storage_get_ticker(i);
        StockData data;

        if (symbol.isEmpty() ||
            !market_service_get_data(symbol, data)) {

            lv_label_set_text(price_labels[i], "--");
            lv_label_set_text(open_labels[i], "--");
            lv_label_set_text(close_labels[i], "--");
            lv_label_set_text(change_labels[i], "--");

            continue;
        }

        char price_text[64];
        char open_text[32];
        char close_text[32];
        char change_text[32];

        // Precio actual
        if (isfinite(data.price)) {
            if (data.currency.length() > 0) {
                snprintf(
                    price_text,
                    sizeof(price_text),
                    "%.2f %s",
                    data.price,
                    data.currency.c_str()
                );
            } else {
                snprintf(
                    price_text,
                    sizeof(price_text),
                    "%.2f",
                    data.price
                );
            }
        } else {
            snprintf(
                price_text,
                sizeof(price_text),
                "--"
            );
        }

        // Precio de apertura
        if (isfinite(data.open)) {
            snprintf(
                open_text,
                sizeof(open_text),
                "%.2f",
                data.open
            );
        } else {
            snprintf(
                open_text,
                sizeof(open_text),
                "--"
            );
        }

        // Cierre de la sesión anterior
        if (isfinite(data.previous_close)) {
            snprintf(
                close_text,
                sizeof(close_text),
                "%.2f",
                data.previous_close
            );
        } else {
            snprintf(
                close_text,
                sizeof(close_text),
                "--"
            );
        }

        lv_label_set_text(price_labels[i], price_text);
        lv_label_set_text(open_labels[i], open_text);
        lv_label_set_text(close_labels[i], close_text);

        // Porcentaje de variación
        if (isfinite(data.change_percent)) {
            snprintf(
                change_text,
                sizeof(change_text),
                "%+.2f%%",
                data.change_percent
            );

            lv_label_set_text(
                change_labels[i],
                change_text
            );

            uint32_t color =
                data.change_percent > 0.001f
                    ? 0x55DD88
                    : data.change_percent < -0.001f
                        ? 0xFF6666
                        : 0xCCCCCC;

            lv_obj_set_style_text_color(
                change_labels[i],
                lv_color_hex(color),
                0
            );
        } else {
            lv_label_set_text(
                change_labels[i],
                "--"
            );

            lv_obj_set_style_text_color(
                change_labels[i],
                lv_color_hex(0xAAAAAA),
                0
            );
        }
    }

    // Estado de actualización
    if (refresh_status != nullptr) {
        if (WiFi.status() != WL_CONNECTED) {
            lv_label_set_text(
                refresh_status,
                "Sin conexion WiFi"
            );
        } else if (market_service_is_busy()) {
            lv_label_set_text(
                refresh_status,
                "Actualizando cotizaciones..."
            );
        } else {
            lv_label_set_text(
                refresh_status,
                "Actualizacion automatica cada 5 min"
            );
        }
    }
}

void list_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *screen =
        static_cast<lv_obj_t *>(timer->user_data);

    // El listado ya no esta activo: detener su temporizador.
    if (lv_scr_act() != screen) {
        if (timer == list_timer) {
            list_timer = nullptr;
        }

        lv_timer_del(timer);
        return;
    }

    update_wifi_indicator();
    update_rows();

    uint32_t now = millis();

    if (!first_refresh_requested ||
        static_cast<uint32_t>(
            now - last_refresh_request
        ) >= AUTO_REFRESH_MS) {

        if (market_service_request_refresh()) {
            first_refresh_requested = true;
            last_refresh_request = now;
        }
    }
}

void clear_row_references()
{
    for (size_t i = 0; i < MAX_ROWS; ++i) {
        ticker_labels[i] = nullptr;
        price_labels[i] = nullptr;
        open_labels[i] = nullptr;
        close_labels[i] = nullptr;
        change_labels[i] = nullptr;
        delete_buttons[i] = nullptr;
    }
}

} // namespace

lv_obj_t *screen_list_create()
{
    if (list_timer != nullptr) {
        lv_timer_del(list_timer);
        list_timer = nullptr;
    }

    clear_row_references();

    wifi_indicator = nullptr;
    refresh_status = nullptr;
    list_screen = nullptr;

    first_refresh_requested = false;
    last_refresh_request = millis();

    lv_obj_t *scr = lv_obj_create(nullptr);
    list_screen = scr;

    lv_obj_set_style_bg_color(
        scr, lv_color_hex(0x121212), 0
    );

    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Barra superior
    lv_obj_t *top_bar = lv_obj_create(scr);

    lv_obj_set_size(top_bar, 800, 65);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_set_style_bg_color(
        top_bar, lv_color_hex(0x1F1F1F), 0
    );

    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 8, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Mi Portafolio");

    lv_obj_set_style_text_color(
        title, lv_color_hex(0xFFFFFF), 0
    );

    lv_obj_set_style_text_font(
        title, &lv_font_montserrat_14, 0
    );

    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    // WiFi
    wifi_indicator = lv_label_create(top_bar);
    lv_obj_set_width(wifi_indicator, 140);

    lv_obj_set_style_text_font(
        wifi_indicator, &lv_font_montserrat_14, 0
    );

    lv_obj_align(
        wifi_indicator, LV_ALIGN_RIGHT_MID, -178, 0
    );

    // Configuración
    lv_obj_t *btn_settings = lv_btn_create(top_bar);
    lv_obj_set_size(btn_settings, 48, 48);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -8, 0);

    lv_obj_set_style_bg_color(
        btn_settings, lv_color_hex(0x333333), 0
    );

    lv_obj_add_event_cb(
        btn_settings,
        settings_btn_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t *lbl_settings = lv_label_create(btn_settings);
    lv_label_set_text(lbl_settings, LV_SYMBOL_SETTINGS);
    lv_obj_center(lbl_settings);

    // Añadir
    lv_obj_t *btn_add = lv_btn_create(top_bar);
    lv_obj_set_size(btn_add, 48, 48);
    lv_obj_align(btn_add, LV_ALIGN_RIGHT_MID, -66, 0);

    lv_obj_set_style_bg_color(
        btn_add, lv_color_hex(0x007ACC), 0
    );

    lv_obj_add_event_cb(
        btn_add,
        add_btn_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t *lbl_add = lv_label_create(btn_add);
    lv_label_set_text(lbl_add, LV_SYMBOL_PLUS);
    lv_obj_center(lbl_add);

    // Refrescar
    lv_obj_t *btn_refresh = lv_btn_create(top_bar);
    lv_obj_set_size(btn_refresh, 48, 48);

    lv_obj_align(
        btn_refresh, LV_ALIGN_RIGHT_MID, -124, 0
    );

    lv_obj_set_style_bg_color(
        btn_refresh, lv_color_hex(0x333333), 0
    );

    lv_obj_add_event_cb(
        btn_refresh,
        refresh_btn_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t *lbl_refresh = lv_label_create(btn_refresh);
    lv_label_set_text(lbl_refresh, LV_SYMBOL_REFRESH);
    lv_obj_center(lbl_refresh);

    // Cabecera fija, fuera del área desplazable
    lv_obj_t *header = lv_obj_create(scr);

    lv_obj_set_size(header, 780, 34);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 70);

    lv_obj_set_style_bg_color(
        header, lv_color_hex(0x303030), 0
    );

    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 5, 0);
    lv_obj_set_style_pad_all(header, 0, 0);

    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Posiciones de las cinco columnas
    lv_obj_t *h_symbol = lv_label_create(header);
    lv_label_set_text(h_symbol, "SIMBOLO");
    lv_obj_set_width(h_symbol, 100);
    lv_obj_set_style_text_color(
        h_symbol, lv_color_hex(0xFFFFFF), 0
    );
    lv_obj_align(h_symbol, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t *h_price = lv_label_create(header);
    lv_label_set_text(h_price, "ACTUAL");
    lv_obj_set_width(h_price, 110);
    lv_obj_set_style_text_color(
        h_price, lv_color_hex(0xFFFFFF), 0
    );
    lv_obj_align(h_price, LV_ALIGN_LEFT_MID, 145, 0);

    lv_obj_t *h_open = lv_label_create(header);
    lv_label_set_text(h_open, "APERTURA");
    lv_obj_set_width(h_open, 110);
    lv_obj_set_style_text_color(
        h_open, lv_color_hex(0xFFFFFF), 0
    );
    lv_obj_align(h_open, LV_ALIGN_LEFT_MID, 290, 0);

    lv_obj_t *h_close = lv_label_create(header);
    lv_label_set_text(h_close, "CIERRE ANT.");
    lv_obj_set_width(h_close, 110);
    lv_obj_set_style_text_color(
        h_close, lv_color_hex(0xFFFFFF), 0
    );
    lv_obj_align(h_close, LV_ALIGN_LEFT_MID, 435, 0);
    
    lv_obj_t *h_change = lv_label_create(header);
    lv_label_set_text(h_change, "CAMBIO");
    lv_obj_set_width(h_change, 75);
    lv_obj_set_style_text_color(
        h_change, lv_color_hex(0xFFFFFF), 0
    );

    // Reservar espacio para el botón de papelera.
    lv_obj_align(
        h_change,
        LV_ALIGN_RIGHT_MID,
        -48,
        0
    );


    // Área desplazable debajo de la cabecera
    list_content = lv_obj_create(scr);

    lv_obj_set_size(list_content, 780, 365);
    lv_obj_align(
        list_content, LV_ALIGN_TOP_MID, 0, 108
    );

    lv_obj_set_style_bg_color(
        list_content, lv_color_hex(0x121212), 0
    );

    lv_obj_set_style_border_width(list_content, 0, 0);
    lv_obj_set_style_pad_all(list_content, 0, 0);

    lv_obj_set_scroll_dir(list_content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(
        list_content, LV_SCROLLBAR_MODE_AUTO
    );

    size_t count = storage_ticker_count();

    if (count > MAX_ROWS) {
        count = MAX_ROWS;
    }

    if (count == 0) {
        lv_obj_t *empty = lv_label_create(list_content);

        lv_label_set_text(
            empty,
            "No hay acciones ni ETFs.\n"
            "Pulsa + para anadir tu primer ticker."
        );

        lv_obj_set_style_text_color(
            empty, lv_color_hex(0xAAAAAA), 0
        );

        lv_obj_align(empty, LV_ALIGN_TOP_MID, 0, 40);
    }

    for (size_t i = 0; i < count; ++i) {
        String symbol = storage_get_ticker(i);

        if (symbol.isEmpty()) {
            continue;
        }

        lv_obj_t *row = lv_obj_create(list_content);

        lv_obj_set_size(row, 780, 48);

        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(
            row,
            ticker_row_event_cb,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(static_cast<uintptr_t>(i))
        );

        lv_obj_align(
            row,
            LV_ALIGN_TOP_LEFT,
            0,
            static_cast<lv_coord_t>(i * 53)
        );

        lv_obj_set_style_bg_color(
            row,
            i % 2 == 0
                ? lv_color_hex(0x1E1E1E)
                : lv_color_hex(0x252525),
            0
        );

        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 5, 0);
        lv_obj_set_style_pad_all(row, 0, 0);

        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        ticker_labels[i] = lv_label_create(row);
        lv_label_set_text(
            ticker_labels[i], symbol.c_str()
        );
        lv_obj_set_width(ticker_labels[i], 100);
        lv_obj_set_style_text_color(
            ticker_labels[i], lv_color_hex(0xFFFFFF), 0
        );
        lv_obj_align(
            ticker_labels[i], LV_ALIGN_LEFT_MID, 8, 0
        );

        price_labels[i] = lv_label_create(row);
        lv_label_set_text(price_labels[i], "--");
        lv_obj_set_width(price_labels[i], 110);
        lv_obj_set_style_text_color(
            price_labels[i], lv_color_hex(0xFFFFFF), 0
        );
        lv_obj_align(
            price_labels[i], LV_ALIGN_LEFT_MID, 145, 0
        );

        open_labels[i] = lv_label_create(row);
        lv_label_set_text(open_labels[i], "--");
        lv_obj_set_width(open_labels[i], 110);
        lv_obj_set_style_text_color(
            open_labels[i], lv_color_hex(0xCCCCCC), 0
        );
        lv_obj_align(
            open_labels[i], LV_ALIGN_LEFT_MID, 290, 0
        );

        close_labels[i] = lv_label_create(row);
        lv_label_set_text(close_labels[i], "--");
        lv_obj_set_width(close_labels[i], 110);
        lv_obj_set_style_text_color(
            close_labels[i], lv_color_hex(0xCCCCCC), 0
        );
        lv_obj_align(
            close_labels[i], LV_ALIGN_LEFT_MID, 435, 0
        );

        
        change_labels[i] = lv_label_create(row);
        lv_label_set_text(change_labels[i], "--");
        lv_obj_set_width(change_labels[i], 75);
        lv_obj_set_style_text_color(
            change_labels[i], lv_color_hex(0xAAAAAA), 0
        );

        // Separar el porcentaje de la papelera.
        lv_obj_align(
            change_labels[i],
            LV_ALIGN_RIGHT_MID,
            -48,
            0
        );


        delete_buttons[i] = lv_btn_create(row);

lv_obj_set_size(delete_buttons[i], 32, 32);

lv_obj_align(
    delete_buttons[i],
    LV_ALIGN_RIGHT_MID,
    -4,
    0
);

lv_obj_set_style_bg_color(
    delete_buttons[i],
    lv_color_hex(0x7A2525),
    0
);

lv_obj_set_style_radius(
    delete_buttons[i],
    6,
    0
);

        lv_obj_add_event_cb(
            delete_buttons[i],
            delete_btn_event_cb,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(static_cast<uintptr_t>(i))
        );

        lv_obj_t *delete_icon = lv_label_create(delete_buttons[i]);

        lv_label_set_text(delete_icon, LV_SYMBOL_TRASH);

        lv_obj_center(delete_icon);

    }

    // Estado de actualización
    refresh_status = lv_label_create(scr);

    lv_obj_set_style_text_color(
        refresh_status, lv_color_hex(0xAAAAAA), 0
    );

    lv_obj_align(
        refresh_status, LV_ALIGN_BOTTOM_LEFT, 15, -7
    );

    update_wifi_indicator();
    update_rows();

    if (market_service_request_refresh()) {
        first_refresh_requested = true;
        last_refresh_request = millis();
    }

    list_timer = lv_timer_create(
        list_timer_cb,
        UI_POLL_MS,
        scr
    );

    return scr;
}
