#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <Arduino.h>

constexpr size_t STORAGE_MAX_TICKERS = 40;

void storage_init();

// Tickers (se mantienen en RAM; NVS solo se toca al modificar).
size_t storage_ticker_count();
String storage_get_ticker(size_t index);
bool storage_ticker_exists(const String &ticker);
bool storage_add_ticker(const String &ticker);
bool storage_remove_ticker(size_t index);
bool storage_clear_tickers();

String storage_normalize_ticker(const String &input);
bool storage_ticker_format_valid(const String &input);

// WiFi.
void storage_save_wifi_credentials(const String &ssid, const String &password);
bool storage_get_wifi_credentials(String &ssid, String &password);
void storage_forget_wifi();

#endif
