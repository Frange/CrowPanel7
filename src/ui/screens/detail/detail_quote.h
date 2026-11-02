#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "../../../services/market_service.h"

lv_obj_t *detail_quote_create(lv_obj_t *parent);

void detail_quote_update(
    const StockData &data,
    bool available
);

void detail_quote_reset();