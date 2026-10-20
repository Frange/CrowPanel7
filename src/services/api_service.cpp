
#include "api_service.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

bool api_fetch_stock_data(const char* symbol, StockData* out)
{
    if (symbol == nullptr || out == nullptr ||
        WiFi.status() != WL_CONNECTED) {
        return false;
    }

    out->symbol = symbol;
    out->market = "";
    out->currency = "";
    out->open = NAN;
    out->high = NAN;
    out->price = NAN;
    out->valid = false;

    String url =
        "https://query1.finance.yahoo.com/v8/finance/chart/";
    url += symbol;
    url += "?range=1d&interval=5m";

    WiFiClientSecure client;
    // Yahoo usa HTTPS. Para producción, es preferible validar
    // el certificado raíz en lugar de desactivar su validación.
    client.setInsecure();
    client.setTimeout(8000);

    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(client, url)) {
        Serial.printf("[YAHOO] No se pudo abrir %s\n", symbol);
        return false;
    }

    http.addHeader("User-Agent", "Mozilla/5.0");
    http.addHeader("Accept", "application/json");

    int code = http.GET();

    if (code != HTTP_CODE_OK) {
        Serial.printf("[YAHOO] %s HTTP %d\n", symbol, code);
        http.end();
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());

    http.end();

    if (err) {
        Serial.printf("[YAHOO] JSON invalido para %s: %s\n",
                      symbol, err.c_str());
        return false;
    }

    JsonVariant result = doc["chart"]["result"][0];

    if (result.isNull()) {
        const char* description =
            doc["chart"]["error"]["description"] | "Sin datos";
        Serial.printf("[YAHOO] %s: %s\n", symbol, description);
        return false;
    }

    JsonVariant meta = result["meta"];

    out->price = meta["regularMarketPrice"] | NAN;
    out->market = meta["fullExchangeName"] |
                  meta["exchangeName"] | "N/D";
    out->currency = meta["currency"] | "";

    JsonVariant quote = result["indicators"]["quote"][0];
    JsonArray opens = quote["open"].as<JsonArray>();
    JsonArray highs = quote["high"].as<JsonArray>();

    // La primera apertura válida de la sesión.
    for (JsonVariant value : opens) {
        if (!value.isNull()) {
            float v = value.as<float>();
            if (isfinite(v) && v > 0) {
                out->open = v;
                break;
            }
        }
    }

    // Máximo observado en las velas intradía.
    for (JsonVariant value : highs) {
        if (!value.isNull()) {
            float v = value.as<float>();
            if (isfinite(v) &&
                (!isfinite(out->high) || v > out->high)) {
                out->high = v;
            }
        }
    }

    // Si no hay velas disponibles, usar el máximo de mercado
    // comunicado por Yahoo, si existe.
    if (!isfinite(out->high)) {
        out->high = meta["regularMarketDayHigh"] | NAN;
    }

    out->valid = isfinite(out->price) && out->price > 0;

    Serial.printf(
        "[YAHOO] %s mercado=%s apertura=%.4f maximo=%.4f actual=%.4f\n",
        symbol,
        out->market.c_str(),
        out->open,
        out->high,
        out->price
    );

    return out->valid;
}
