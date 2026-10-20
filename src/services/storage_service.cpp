#include "storage_service.h"
#include <Preferences.h>

Preferences preferences;

void storage_init() {
    preferences.begin("stocks", false);
}

std::vector<String> storage_get_tickers() {
    String saved = preferences.getString("tickers", "AAPL,VWCE.DE,MSFT");
    std::vector<String> list;
    int start = 0;
    int end = saved.indexOf(',');
    while (end != -1) {
        list.push_back(saved.substring(start, end));
        start = end + 1;
        end = saved.indexOf(',', start);
    }
    if (start < (int)saved.length()) {
        list.push_back(saved.substring(start));
    }
    return list;
}

void storage_save_tickers(const std::vector<String>& tickers) {
    String saved = "";
    for (size_t i = 0; i < tickers.size(); i++) {
        saved += tickers[i];
        if (i < tickers.size() - 1) saved += ",";
    }
    preferences.putString("tickers", saved);
}