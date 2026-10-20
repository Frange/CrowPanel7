#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <Arduino.h>
#include <vector>

void storage_init();
std::vector<String> storage_get_tickers();
void storage_save_tickers(const std::vector<String>& tickers);

#endif