#ifndef API_SERVICE_H
#define API_SERVICE_H

#include <Arduino.h>

bool api_fetch_stock_data(const char* symbol, float *out_price, float *out_change);

#endif