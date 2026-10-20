
#pragma once

#include <Arduino.h>
#include <stdint.h>

struct StockData {
    String symbol;
    String market;
    String currency;

    float open;
    float high;
    float low;
    float price;
    float previous_close;
    float change;
    float change_percent;

    uint32_t updated_at;
    bool valid;
};

// Consulta los datos de un ticker.
// Devuelve true si obtiene un precio valido.
bool api_fetch_stock_data(const char *symbol, StockData *out);
