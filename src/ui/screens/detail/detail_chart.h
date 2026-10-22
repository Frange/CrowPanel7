#pragma once

#include <Arduino.h>
#include <lvgl.h>

void detail_chart_create(
    lv_obj_t *parent,
    const String &symbol
);

void detail_chart_set_symbol(
    const String &symbol
);

void detail_chart_destroy();