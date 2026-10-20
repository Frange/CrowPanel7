#ifndef API_SERVICE_H
#define API_SERVICE_H

#include <Arduino.h>
#include "../models/stock_model.h"

void api_init_wifi(const char* ssid, const char* password);
bool api_fetch_stock_data(const String& symbol, StockData& outStock);

#endif