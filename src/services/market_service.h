#pragma once

#include <Arduino.h>
#include "api_service.h"

void market_service_init();

bool market_service_request_refresh();
bool market_service_is_busy();

// Se incrementa cada vez que cambia algun dato en cache.
// La UI lo compara para repintar solo cuando hay algo nuevo.
uint32_t market_service_data_version();

// millis() de la ultima ronda de actualizacion completada (0 si ninguna).
uint32_t market_service_last_refresh_ms();

bool market_service_get_data(
    const String &symbol,
    StockData &out
);

void market_service_clear_cache();

bool market_service_validate_async(
    const String &symbol
);

bool market_service_take_validation_result(
    String &symbol,
    StockData &data,
    bool &success
);

bool market_service_request_history(
    const String &symbol,
    HistoryPeriod period
);

bool market_service_take_history_result(
    HistoryData &out
);
