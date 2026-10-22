#pragma once

#include <Arduino.h>
#include <stdint.h>

constexpr size_t HISTORY_MAX_POINTS = 120;

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

enum HistoryPeriod : uint8_t {
    HISTORY_1D = 0,
    HISTORY_1W,
    HISTORY_1M,
    HISTORY_6M,
    HISTORY_1Y,
    HISTORY_5Y,
    HISTORY_PERIOD_COUNT
};

struct HistoryPoint {
    uint32_t timestamp;
    float close;
};

struct HistoryData {
    String symbol;
    HistoryPeriod period;
    HistoryPoint points[HISTORY_MAX_POINTS];
    uint16_t count;
    bool valid;
};

bool api_fetch_stock_data(
    const char *symbol,
    StockData *out
);

bool api_fetch_stock_history(
    const char *symbol,
    HistoryPeriod period,
    HistoryData *out
);