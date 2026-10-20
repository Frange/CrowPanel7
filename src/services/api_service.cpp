#include "api_service.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool api_fetch_stock_data(const char* symbol, float *out_price, float *out_change) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = "https://api.example.com/quote?symbol=" + String(symbol);
    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            *out_price = doc["price"] | 0.0f;
            *out_change = doc["change"] | 0.0f;
            http.end();
            return true;
        }
    }

    http.end();
    return false;
}