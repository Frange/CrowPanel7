#pragma once

#include <lvgl.h>
#include <stddef.h>
#include <Arduino.h>

lv_obj_t *screen_list_header_create(lv_obj_t *parent);

lv_obj_t *screen_list_row_create(
    lv_obj_t *parent,
    size_t index,
    const String &symbol
);