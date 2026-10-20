
#include "api_service.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

static float json_float_or_nan(JsonVariantConst value)
{
    if (value.isNull()) {
        return NAN;
    }

    float result = value.as<float>();
    return isfinite(result) ? result : NAN;
}

bool api_fetch_stock_data(const char *symbol, StockData *out)
{
    if (out == nullptr) {
        return false;
    }

    out->symbol = symbol != nullptr ? symbol : "";
    out->market = "";
    out->currency = "";
    out->open = NAN;
    out->high = NAN;
    out->low = NAN;
    out->price = NAN;
    out->previous_close = NAN;
    out->change = NAN;
    out->change_percent = NAN;
    out->updated_at = 0;
    out->valid = false;

    if (symbol == nullptr || symbol[0] == '\0') {
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("[YAHOO] %s: sin conexion WiFi\n", symbol);
        return false;
    }

    String url =
        "https://query1.finance.yahoo.com/v8/finance/chart/";

    url += symbol;
    url += "?range=1d&interval=5m";

    WiFiClientSecure client;

    // Solo para pruebas. En produccion, validar el certificado TLS.
    client.setInsecure();
    client.setTimeout(10000);

    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(client, url)) {
        Serial.printf(
            "[YAHOO] %s: no se pudo iniciar HTTP\n",
            symbol
        );
        return false;
    }

    http.addHeader(
        "User-Agent",
        "Mozilla/5.0 (X11; Linux x86_64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/131.0.0.0 Safari/537.36"
    );
    http.addHeader("Accept", "application/json");

    int code = http.GET();

    if (code != HTTP_CODE_OK) {
        Serial.printf("[YAHOO] %s: HTTP %d\n", symbol, code);

        if (code > 0) {
            String body = http.getString();
            Serial.printf(
                "[YAHOO] Respuesta: %s\n",
                body.substring(0, 220).c_str()
            );
        }

        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    if (body.isEmpty()) {
        Serial.printf(
            "[YAHOO] %s: respuesta vacia\n",
            symbol
        );
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        Serial.printf(
            "[YAHOO] %s: JSON invalido (%s), bytes=%u\n",
            symbol,
            error.c_str(),
            static_cast<unsigned int>(body.length())
        );

        Serial.printf(
            "[YAHOO] Inicio respuesta: %s\n",
            body.substring(0, 220).c_str()
        );

        return false;
    }

    JsonVariant result = doc["chart"]["result"][0];

    if (result.isNull()) {
        const char *description =
            doc["chart"]["error"]["description"] |
            "Yahoo no devolvio datos para el simbolo";

        Serial.printf(
            "[YAHOO] %s: %s\n",
            symbol,
            description
        );
        return false;
    }

    JsonVariant meta = result["meta"];

    const char *returned_symbol = meta["symbol"] | "";

    if (returned_symbol[0] != '\0' &&
        String(returned_symbol) != String(symbol)) {
        Serial.printf(
            "[YAHOO] Simbolo solicitado=%s, recibido=%s\n",
            symbol,
            returned_symbol
        );
        return false;
    }

    out->market =
        meta["fullExchangeName"] |
        meta["exchangeName"] |
        "N/D";

    out->currency = meta["currency"] | "";

    out->price = json_float_or_nan(
        meta["regularMarketPrice"]
    );

    out->previous_close = json_float_or_nan(
        meta["chartPreviousClose"]
    );

    if (!isfinite(out->previous_close)) {
        out->previous_close = json_float_or_nan(
            meta["previousClose"]
        );
    }

    JsonVariant quote =
        result["indicators"]["quote"][0];

    JsonArray opens = quote["open"].as<JsonArray>();
    JsonArray highs = quote["high"].as<JsonArray>();
    JsonArray lows = quote["low"].as<JsonArray>();
    JsonArray closes = quote["close"].as<JsonArray>();

    // Primera apertura disponible de la sesion.
    for (JsonVariant value : opens) {
        float v = json_float_or_nan(value);

        if (isfinite(v) && v > 0) {
            out->open = v;
            break;
        }
    }

    // Maximo observado en las velas intradia.
    for (JsonVariant value : highs) {
        float v = json_float_or_nan(value);

        if (isfinite(v) && v > 0 &&
            (!isfinite(out->high) || v > out->high)) {
            out->high = v;
        }
    }

    // Minimo observado en las velas intradia.
    for (JsonVariant value : lows) {
        float v = json_float_or_nan(value);

        if (isfinite(v) && v > 0 &&
            (!isfinite(out->low) || v < out->low)) {
            out->low = v;
        }
    }

    float last_close = NAN;

    for (JsonVariant value : closes) {
        float v = json_float_or_nan(value);

        if (isfinite(v) && v > 0) {
            last_close = v;
        }
    }

    // Algunos mercados no proporcionan regularMarketPrice.
    // En ese caso usamos el cierre de la ultima vela disponible.
    if (!isfinite(out->price) || out->price <= 0) {
        if (isfinite(last_close) && last_close > 0) {
            out->price = last_close;

            Serial.printf(
                "[YAHOO] %s: usando ultimo cierre de vela\n",
                symbol
            );
        }
    }

    // Completar maximo/minimo con los metadatos del mercado.
    if (!isfinite(out->high)) {
        out->high = json_float_or_nan(
            meta["regularMarketDayHigh"]
        );
    }

    if (!isfinite(out->low)) {
        out->low = json_float_or_nan(
            meta["regularMarketDayLow"]
        );
    }

    if (!isfinite(out->previous_close) ||
        out->previous_close <= 0) {
        out->previous_close = json_float_or_nan(
            meta["previousClose"]
        );
    }

    if (!isfinite(out->price) || out->price <= 0) {
        Serial.printf(
            "[YAHOO] %s: sin precio valido\n",
            symbol
        );
        return false;
    }

    if (isfinite(out->previous_close) &&
        out->previous_close > 0) {
        out->change = out->price - out->previous_close;

        out->change_percent =
            (out->change / out->previous_close) * 100.0f;
    }

    out->updated_at = millis();
    out->valid = true;

    Serial.printf(
        "[YAHOO] %s | Mercado=%s | Moneda=%s | "
        "Actual=%.4f | Apertura=%.4f | CierreAnt=%.4f | "
        "Max=%.4f | Min=%.4f | Cambio=%.2f%%\n",
        symbol,
        out->market.c_str(),
        out->currency.c_str(),
        out->price,
        out->open,
        out->previous_close,
        out->high,
        out->low,
        out->change_percent
    );

    return true;
}
