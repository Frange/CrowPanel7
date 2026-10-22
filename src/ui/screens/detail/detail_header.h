#pragma once

#include <Arduino.h>
#include <lvgl.h>

lv_obj_t *detail_header_create(
    lv_obj_t *parent,
    const String &symbol
);

void detail_header_set_symbol(const String &symbol);
void detail_header_reset();
