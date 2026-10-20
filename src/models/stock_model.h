#ifndef STOCK_MODEL_H
#define STOCK_MODEL_H

#include <Arduino.h>
#include <vector>

struct StockData {
    String symbol;        // ej: "AAPL", "VWCE.DE"
    String name;          // ej: "Apple Inc."
    float currentPrice;
    float changePercent;
    std::vector<float> history1D;
    std::vector<float> history1M;
    std::vector<float> history1Y;
    std::vector<float> history5Y;
};

#endif