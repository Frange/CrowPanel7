#include "api_service.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void api_init_wifi(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado!");
}

bool api_fetch_stock_data(const String& symbol, StockData& outStock) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    // Endpoint para obtener datos de cotización e históricos (ej: interval=1d, range=1y)
    String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol + "?interval=1d&range=1y";
    
    http.begin(url);
    http.setUserAgent("Mozilla/5.0"); // Requerido por Yahoo
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parsear JSON (usando JsonDocument de ArduinoJson v7)
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) return false;

    JsonObject result = doc["chart"]["result"][0];
    outStock.symbol = symbol;
    outStock.name = result["meta"]["shortName"].as<String>();
    outStock.currentPrice = result["meta"]["regularMarketPrice"].as<float>();

    // Extraer precios de cierre para la gráfica (1Y como ejemplo)
    outStock.history1Y.clear();
    JsonArray closePrices = result["indicators"]["quote"][0]["close"];
    for (JsonVariant val : closePrices) {
        if (!val.isNull()) {
            outStock.history1Y.push_back(val.as<float>());
        }
    }

    return true;
}