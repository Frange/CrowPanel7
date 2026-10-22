
#include "api_service.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <stdio.h>

namespace {

// --------------------------------------------------
// Conversión segura de valores JSON a float
// --------------------------------------------------

float json_float_or_nan(JsonVariantConst value)
{
    if (value.isNull()) {
        return NAN;
    }

    float result = value.as<float>();

    return isfinite(result) ? result : NAN;
}

// --------------------------------------------------
// Descarga y validación del JSON de Yahoo Finance
// --------------------------------------------------

bool download_json(
    const String &url,
    JsonDocument &doc,
    const char *symbol)
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf(
            "[YAHOO] %s: sin WiFi\n",
            symbol
        );
        return false;
    }

    WiFiClientSecure client;

    // Solo para pruebas. En producción, validar el certificado TLS.
    client.setInsecure();
    client.setTimeout(12000);

    HTTPClient http;

    http.setConnectTimeout(10000);
    http.setTimeout(12000);

    http.setFollowRedirects(
        HTTPC_STRICT_FOLLOW_REDIRECTS
    );

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
        "AppleWebKit/537.36 Chrome/131.0.0.0 Safari/537.36"
    );

    http.addHeader(
        "Accept",
        "application/json"
    );

    int code = http.GET();

    if (code != HTTP_CODE_OK) {
        Serial.printf(
            "[YAHOO] %s: HTTP %d\n",
            symbol,
            code
        );

        if (code > 0) {
            String response = http.getString();

            Serial.printf(
                "[YAHOO] %s: respuesta HTTP: %.250s\n",
                symbol,
                response.c_str()
            );
        }

        http.end();
        return false;
    }

    // Leemos el cuerpo antes de cerrar HTTP.
    // Así podemos diagnosticar respuestas que no sean JSON.
    String body = http.getString();

    http.end();

    if (body.isEmpty()) {
        Serial.printf(
            "[YAHOO] %s: respuesta vacia\n",
            symbol
        );
        return false;
    }

    DeserializationError error =
        deserializeJson(doc, body);

    if (error) {
        Serial.printf(
            "[YAHOO] %s: JSON invalido (%s), HTTP=%d, bytes=%u\n",
            symbol,
            error.c_str(),
            code,
            static_cast<unsigned int>(body.length())
        );

        // Solo imprimimos el principio para no llenar
        // el monitor serie con respuestas muy grandes.
        Serial.printf(
            "[YAHOO] %s: inicio respuesta: %.300s\n",
            symbol,
            body.c_str()
        );

        return false;
    }

    JsonVariantConst result =
        doc["chart"]["result"][0];

    if (result.isNull()) {
        const char *description =
            doc["chart"]["error"]["description"] |
            "Yahoo no devolvio datos";

        Serial.printf(
            "[YAHOO] %s: %s\n",
            symbol,
            description
        );

        return false;
    }

    const char *returned_symbol =
        result["meta"]["symbol"] | "";

    if (returned_symbol[0] != '\0' &&
        String(returned_symbol) != String(symbol)) {
        Serial.printf(
            "[YAHOO] Simbolo solicitado=%s, recibido=%s\n",
            symbol,
            returned_symbol
        );

        return false;
    }

    return true;
}

// --------------------------------------------------
// Parámetros de histórico por período
// --------------------------------------------------

bool period_parameters(
    HistoryPeriod period,
    const char **range,
    const char **interval)
{
    switch (period) {
        case HISTORY_1D:
            *range = "1d";
            *interval = "5m";
            return true;

        case HISTORY_1W:
            *range = "5d";
            *interval = "15m";
            return true;

        case HISTORY_1M:
            *range = "1mo";
            *interval = "1d";
            return true;

        case HISTORY_6M:
            *range = "6mo";
            *interval = "1d";
            return true;

        case HISTORY_1Y:
            *range = "1y";
            *interval = "1d";
            return true;

        case HISTORY_5Y:
            *range = "5y";
            *interval = "1wk";
            return true;

        default:
            return false;
    }
}

} // namespace

// ==================================================
// COTIZACIÓN ACTUAL
// ==================================================

bool api_fetch_stock_data(
    const char *symbol,
    StockData *out)
{
    if (out == nullptr) {
        return false;
    }

    *out = {};

    out->symbol = symbol != nullptr ? symbol : "";
    out->open = NAN;
    out->high = NAN;
    out->low = NAN;
    out->price = NAN;
    out->previous_close = NAN;
    out->change = NAN;
    out->change_percent = NAN;

    if (symbol == nullptr || symbol[0] == '\0') {
        return false;
    }

    String url =
        "https://query1.finance.yahoo.com/v8/finance/chart/";

    url += symbol;
    url += "?range=1d&interval=5m";

    JsonDocument doc;

    if (!download_json(url, doc, symbol)) {
        return false;
    }

    JsonVariantConst result =
        doc["chart"]["result"][0];

    JsonVariantConst meta =
        result["meta"];

    out->symbol =
        meta["symbol"] | symbol;

    out->market =
        meta["fullExchangeName"] |
        meta["exchangeName"] |
        "N/D";

    out->currency =
        meta["currency"] | "";

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

    JsonVariantConst quote =
        result["indicators"]["quote"][0];

    JsonArrayConst opens =
        quote["open"].as<JsonArrayConst>();

    JsonArrayConst highs =
        quote["high"].as<JsonArrayConst>();

    JsonArrayConst lows =
        quote["low"].as<JsonArrayConst>();

    JsonArrayConst closes =
        quote["close"].as<JsonArrayConst>();

    // Primera apertura disponible.
    for (JsonVariantConst value : opens) {
        float price = json_float_or_nan(value);

        if (isfinite(price) && price > 0) {
            out->open = price;
            break;
        }
    }

    // Máximo intradía.
    for (JsonVariantConst value : highs) {
        float price = json_float_or_nan(value);

        if (isfinite(price) &&
            price > 0 &&
            (!isfinite(out->high) || price > out->high)) {
            out->high = price;
        }
    }

    // Mínimo intradía.
    for (JsonVariantConst value : lows) {
        float price = json_float_or_nan(value);

        if (isfinite(price) &&
            price > 0 &&
            (!isfinite(out->low) || price < out->low)) {
            out->low = price;
        }
    }

    // Último cierre válido.
    float last_close = NAN;

    for (JsonVariantConst value : closes) {
        float price = json_float_or_nan(value);

        if (isfinite(price) && price > 0) {
            last_close = price;
        }
    }

    // Si no hay precio de mercado, usar el último cierre.
    if (!isfinite(out->price) || out->price <= 0) {
        out->price = last_close;
    }

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
        out->change =
            out->price - out->previous_close;

        out->change_percent =
            (out->change / out->previous_close) * 100.0f;
    }

    out->updated_at = millis();
    out->valid = true;

    Serial.printf(
        "[YAHOO] %s: precio=%.4f cambio=%.2f%%\n",
        symbol,
        out->price,
        out->change_percent
    );

    return true;
}

// ==================================================
// HISTÓRICO PARA EL GRÁFICO
// ==================================================

bool api_fetch_stock_history(
    const char *symbol,
    HistoryPeriod period,
    HistoryData *out)
{
    if (out == nullptr) {
        return false;
    }

    *out = {};

    out->symbol =
        symbol != nullptr ? symbol : "";

    out->period = period;

    if (symbol == nullptr || symbol[0] == '\0') {
        return false;
    }

    const char *range = nullptr;
    const char *interval = nullptr;

    if (!period_parameters(
            period,
            &range,
            &interval)) {
        Serial.printf(
            "[YAHOO] %s: periodo invalido\n",
            symbol
        );

        return false;
    }

    String url =
        "https://query1.finance.yahoo.com/v8/finance/chart/";

    url += symbol;
    url += "?range=";
    url += range;
    url += "&interval=";
    url += interval;

    JsonDocument doc;

    if (!download_json(url, doc, symbol)) {
        return false;
    }

    JsonVariantConst result =
        doc["chart"]["result"][0];

    JsonArrayConst timestamps =
        result["timestamp"].as<JsonArrayConst>();

    JsonArrayConst closes =
        result["indicators"]["quote"][0]["close"]
            .as<JsonArrayConst>();

    size_t total = timestamps.size();

    if (closes.size() < total) {
        total = closes.size();
    }

    if (total == 0) {
        Serial.printf(
            "[YAHOO] %s: historico vacio\n",
            symbol
        );

        return false;
    }

    // Contar cierres válidos.
    size_t valid_count = 0;

    for (size_t i = 0; i < total; ++i) {
        float price = json_float_or_nan(closes[i]);

        if (isfinite(price) && price > 0) {
            ++valid_count;
        }
    }

    if (valid_count == 0) {
        Serial.printf(
            "[YAHOO] %s: ningun cierre valido\n",
            symbol
        );

        return false;
    }

    // Reducir el número de puntos si hace falta.
    // Se conserva el primer punto, muestras uniformes
    // y el último punto válido.
    size_t stride = 1;

    if (valid_count > HISTORY_MAX_POINTS) {
        stride =
            (valid_count - 1) /
            (HISTORY_MAX_POINTS - 1);

        if (stride == 0) {
            stride = 1;
        }
    }

    size_t valid_index = 0;
    size_t next_sample = 0;

    for (size_t i = 0; i < total; ++i) {
        float price = json_float_or_nan(closes[i]);

        if (!isfinite(price) || price <= 0) {
            continue;
        }

        bool is_last =
            valid_index == valid_count - 1;

        if (valid_index == next_sample || is_last) {
            if (out->count < HISTORY_MAX_POINTS) {
                out->points[out->count].timestamp =
                    timestamps[i].as<uint32_t>();

                out->points[out->count].close = price;

                ++out->count;
            }

            next_sample += stride;
        }

        ++valid_index;
    }

    out->valid = out->count > 0;

    Serial.printf(
        "[YAHOO] Historico %s periodo=%u puntos=%u\n",
        symbol,
        static_cast<unsigned int>(period),
        static_cast<unsigned int>(out->count)
    );

    return out->valid;
}
