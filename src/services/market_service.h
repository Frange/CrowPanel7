#pragma once

#include <Arduino.h>
#include "api_service.h"

void market_service_init();

bool market_service_request_refresh();
bool market_service_is_busy();

bool market_service_get_data(
    const String &symbol,
    StockData &out
);

bool market_service_validate_async(
    const String &symbol
);

bool market_service_take_validation_result(
    String &symbol,
    StockData &data,
    bool &success
);