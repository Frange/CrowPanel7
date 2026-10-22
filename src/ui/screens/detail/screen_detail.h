#pragma once

#include <Arduino.h>
#include <lvgl.h>

// Establece el símbolo que se mostrará en el detalle.
void screen_detail_set_symbol(const String &symbol);

// Crea y devuelve la pantalla de detalle.
lv_obj_t *screen_detail_create();