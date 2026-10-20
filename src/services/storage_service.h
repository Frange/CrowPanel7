#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <Arduino.h>

enum StorageNetworkStatus {
    STORAGE_WIFI_DISCONNECTED = 0,
    STORAGE_WIFI_NO_INTERNET = 1,
    STORAGE_WIFI_INTERNET = 2
};

void storage_init();

size_t storage_ticker_count();
String storage_get_ticker(size_t index);
bool storage_ticker_exists(const String &ticker);
bool storage_add_ticker(const String &ticker);

void storage_save_wifi_credentials(const String &ssid, const String &password);
bool storage_get_wifi_credentials(String &ssid, String &password);
int storage_wifi_status();

#endif
