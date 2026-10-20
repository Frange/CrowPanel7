#ifndef STOCK_MODEL_H
#define STOCK_MODEL_H

#include <Arduino.h>

struct StockItem {
    char symbol[16];
    char name[32];
    float price;
    float change;
};

#endif