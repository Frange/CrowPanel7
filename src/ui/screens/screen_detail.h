#ifndef SCREEN_DETAIL_H
#define SCREEN_DETAIL_H

#include <lvgl.h>
#include <Arduino.h>

lv_obj_t *screen_detail_create();

void screen_detail_set_symbol(const String &symbol);

#endif