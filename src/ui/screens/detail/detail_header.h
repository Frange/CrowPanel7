#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "../../../services/market_service.h"

lv_obj_t *detail_header_create(
    lv_obj_t *parent,
    const String &symbol
);

// Muestra el nombre de la empresa y el mercado bajo el simbolo.
void detail_header_set_info(const StockData &data, bool available);

void detail_header_reset();
