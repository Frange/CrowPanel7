#ifndef API_SERVICE_H
#define API_SERVICE_H

#include <Arduino.h>

struct StockData {
    String symbol;
    String market;
    String currency;
    float open;
    float high;
    float price;
    bool valid;
};

bool api_fetch_stock_data(const char* symbol, StockData* out);

#endif