
#include "detail_chart.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "../../../services/market_service.h"

namespace {

constexpr int PERIOD_COUNT = HISTORY_PERIOD_COUNT;
constexpr uint32_t HISTORY_POLL_MS = 100;

const char *period_names[PERIOD_COUNT] = {
    "1D", "1S", "1M", "6M", "1A", "5A"
};

const HistoryPeriod period_values[PERIOD_COUNT] = {
    HISTORY_1D,
    HISTORY_1W,
    HISTORY_1M,
    HISTORY_6M,
    HISTORY_1Y,
    HISTORY_5Y
};

lv_obj_t *chart = nullptr;
lv_obj_t *status_label = nullptr;
lv_obj_t *value_label = nullptr;
lv_obj_t *period_buttons[PERIOD_COUNT] = {};

lv_chart_series_t *series = nullptr;
lv_timer_t *history_timer = nullptr;

String selected_symbol;
String requested_symbol;

int selected_period = 0;
HistoryPeriod requested_period = HISTORY_1D;

bool request_in_flight = false;

void show_status(const char *message)
{
    if (status_label == nullptr) {
        return;
    }

    lv_label_set_text(status_label, message);
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
}

void hide_status()
{
    if (status_label != nullptr) {
        lv_obj_add_flag(
            status_label,
            LV_OBJ_FLAG_HIDDEN
        );
    }
}

void update_selected_button()
{
    for (int i = 0; i < PERIOD_COUNT; ++i) {
        if (period_buttons[i] == nullptr) {
            continue;
        }

        lv_obj_set_style_bg_color(
            period_buttons[i],
            i == selected_period
                ? lv_color_hex(0x007ACC)
                : lv_color_hex(0x303030),
            LV_PART_MAIN
        );
    }
}

void clear_chart()
{
    if (chart == nullptr || series == nullptr) {
        return;
    }

    lv_chart_set_all_value(
        chart,
        series,
        LV_CHART_POINT_NONE
    );

    lv_chart_refresh(chart);
}

void request_selected_history()
{
    if (request_in_flight) {
        return;
    }

    if (selected_symbol.isEmpty()) {
        show_status("No hay simbolo seleccionado");
        return;
    }

    HistoryPeriod period =
        period_values[selected_period];

    char message[80];

    snprintf(
        message,
        sizeof(message),
        "Descargando historico %s...",
        period_names[selected_period]
    );

    show_status(message);

    if (!market_service_request_history(
            selected_symbol,
            period)) {
        show_status(
            "No se pudo solicitar el historico"
        );

        Serial.println(
            "[DETAIL_CHART] Solicitud de historico rechazada"
        );

        return;
    }

    requested_symbol = selected_symbol;
    requested_period = period;
    request_in_flight = true;

    Serial.printf(
        "[DETAIL_CHART] Solicitado %s periodo=%s\n",
        selected_symbol.c_str(),
        period_names[selected_period]
    );
}


void show_history(const HistoryData &data)
{
    if (chart == nullptr || series == nullptr) {
        return;
    }

    if (!data.valid || data.count == 0) {
        clear_chart();
        show_status("No hay historico disponible");
        return;
    }

    float min_price = INFINITY;
    float max_price = -INFINITY;

    for (uint16_t i = 0; i < data.count; ++i) {
        float price = data.points[i].close;

        if (!isfinite(price) || price <= 0) {
            continue;
        }

        if (price < min_price) {
            min_price = price;
        }

        if (price > max_price) {
            max_price = price;
        }
    }

    if (!isfinite(min_price) ||
        !isfinite(max_price)) {
        clear_chart();
        show_status("No hay precios validos");
        return;
    }

    float span = max_price - min_price;

    float padding = span > 0.0f
        ? span * 0.08f
        : max_price * 0.01f;

    if (padding <= 0.0f) {
        padding = 0.01f;
    }

    constexpr float SCALE = 1000.0f;

    int32_t range_min = static_cast<int32_t>(
        floorf((min_price - padding) * SCALE)
    );

    int32_t range_max = static_cast<int32_t>(
        ceilf((max_price + padding) * SCALE)
    );

    if (range_max <= range_min) {
        range_max = range_min + 1;
    }

    lv_chart_set_point_count(
        chart,
        data.count
    );

    lv_chart_set_range(
        chart,
        LV_CHART_AXIS_PRIMARY_Y,
        static_cast<lv_coord_t>(range_min),
        static_cast<lv_coord_t>(range_max)
    );

    lv_chart_set_all_value(
        chart,
        series,
        LV_CHART_POINT_NONE
    );

    uint16_t plotted = 0;
    float last_price = NAN;

    for (uint16_t i = 0; i < data.count; ++i) {
        float price = data.points[i].close;

        if (!isfinite(price) || price <= 0) {
            continue;
        }

        int32_t scaled = static_cast<int32_t>(
            lroundf(price * SCALE)
        );

        lv_chart_set_next_value(
            chart,
            series,
            static_cast<lv_coord_t>(scaled)
        );

        last_price = price;
        ++plotted;
    }

    if (plotted == 0) {
        clear_chart();
        show_status("No hay puntos validos");
        return;
    }

    lv_chart_refresh(chart);
    hide_status();

    if (value_label != nullptr &&
        isfinite(last_price)) {
        char text[80];

        snprintf(
            text,
            sizeof(text),
            "Ultimo cierre: %.2f",
            last_price
        );

        lv_label_set_text(value_label, text);
    }

    Serial.printf(
        "[DETAIL_CHART] Dibujados %u puntos para %s\n",
        static_cast<unsigned int>(plotted),
        data.symbol.c_str()
    );
}

void history_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!request_in_flight) {
        return;
    }

    HistoryData result = {};

    if (!market_service_take_history_result(result)) {
        return;
    }

    request_in_flight = false;

    bool result_matches =
        result.symbol == requested_symbol &&
        result.period == requested_period;

    bool selection_matches =
        selected_symbol == requested_symbol &&
        period_values[selected_period] == requested_period;

    if (result_matches && selection_matches) {
        if (result.valid && result.count > 0) {
            show_history(result);
        } else {
            clear_chart();
            show_status(
                "Error descargando historico de Yahoo"
            );
        }

        return;
    }

    if (selected_symbol != requested_symbol ||
        period_values[selected_period] != requested_period) {
        request_selected_history();
    }
}


void period_event_cb(lv_event_t *event)
{
    int period = static_cast<int>(
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event)
        )
    );

    if (period < 0 || period >= PERIOD_COUNT) {
        return;
    }

    if (period == selected_period) {
        return;
    }

    selected_period = period;
    update_selected_button();

    if (request_in_flight) {
        char message[80];

        snprintf(
            message,
            sizeof(message),
            "Esperando para cargar %s...",
            period_names[selected_period]
        );

        show_status(message);
        return;
    }

    request_selected_history();
}

lv_obj_t *make_period_button(
    lv_obj_t *parent,
    int index)
{
    lv_obj_t *button = lv_btn_create(parent);

    lv_obj_set_size(button, 82, 36);

    lv_obj_align(
        button,
        LV_ALIGN_TOP_MID,
        (index - 2) * 91,
        194
    );

    lv_obj_set_style_bg_color(
        button,
        index == selected_period
            ? lv_color_hex(0x007ACC)
            : lv_color_hex(0x303030),
        LV_PART_MAIN
    );

    lv_obj_add_event_cb(
        button,
        period_event_cb,
        LV_EVENT_CLICKED,
        reinterpret_cast<void *>(
            static_cast<uintptr_t>(index)
        )
    );

    lv_obj_t *label = lv_label_create(button);

    lv_label_set_text(
        label,
        period_names[index]
    );

    lv_obj_center(label);

    return button;
}

} // namespace

void detail_chart_create(
    lv_obj_t *parent,
    const String &symbol)
{
    selected_symbol = symbol;
    selected_period = 0;

    requested_symbol = "";
    requested_period = HISTORY_1D;
    request_in_flight = false;

    for (int i = 0; i < PERIOD_COUNT; ++i) {
        period_buttons[i] = make_period_button(
            parent,
            i
        );
    }

    chart = lv_chart_create(parent);

    lv_obj_set_size(chart, 760, 175);

    lv_obj_align(
        chart,
        LV_ALIGN_TOP_MID,
        0,
        238
    );

    lv_chart_set_type(
        chart,
        LV_CHART_TYPE_LINE
    );

    lv_chart_set_point_count(chart, 2);

    lv_chart_set_range(
        chart,
        LV_CHART_AXIS_PRIMARY_Y,
        0,
        100
    );

    lv_obj_set_style_pad_all(
        chart,
        10,
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_color(
        chart,
        lv_color_hex(0x181818),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        chart,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        chart,
        1,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_color(
        chart,
        lv_color_hex(0x303030),
        LV_PART_MAIN
    );

    lv_obj_set_style_radius(
        chart,
        8,
        LV_PART_MAIN
    );

    lv_obj_set_style_line_color(
        chart,
        lv_color_hex(0x303030),
        LV_PART_MAIN
    );

    lv_chart_set_div_line_count(chart, 4, 0);

    series = lv_chart_add_series(
        chart,
        lv_color_hex(0x55DD88),
        LV_CHART_AXIS_PRIMARY_Y
    );

    status_label = lv_label_create(parent);

    lv_label_set_text(status_label, "");

    lv_obj_set_width(status_label, 700);

    lv_obj_set_style_text_color(
        status_label,
        lv_color_hex(0xAAAAAA),
        0
    );

    lv_obj_set_style_text_align(
        status_label,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_align(
        status_label,
        LV_ALIGN_TOP_MID,
        0,
        418
    );

    value_label = lv_label_create(parent);

    lv_label_set_text(
        value_label,
        "Ultimo cierre: --"
    );

    lv_obj_set_style_text_color(
        value_label,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_align(
        value_label,
        LV_ALIGN_TOP_MID,
        0,
        445
    );

    history_timer = lv_timer_create(
        history_timer_cb,
        HISTORY_POLL_MS,
        nullptr
    );

    request_selected_history();
}

void detail_chart_set_symbol(const String &symbol)
{
    if (selected_symbol == symbol) {
        return;
    }

    selected_symbol = symbol;

    clear_chart();

    if (value_label != nullptr) {
        lv_label_set_text(
            value_label,
            "Ultimo cierre: --"
        );
    }

    if (request_in_flight) {
        show_status("Esperando cambio de simbolo...");
        return;
    }

    request_selected_history();
}
