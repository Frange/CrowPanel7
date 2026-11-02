#include "storage_service.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {

Preferences preferences;
bool storage_ready = false;

// Copia en RAM de los tickers. La UI y el worker de mercado la leen,
// asi que se protege con un mutex.
String tickers[STORAGE_MAX_TICKERS];
size_t ticker_count = 0;
SemaphoreHandle_t tickers_mutex = nullptr;

struct Lock {
    Lock() {
        if (tickers_mutex != nullptr) {
            xSemaphoreTake(tickers_mutex, portMAX_DELAY);
        }
    }
    ~Lock() {
        if (tickers_mutex != nullptr) {
            xSemaphoreGive(tickers_mutex);
        }
    }
};

void ticker_key(char *out, size_t size, size_t index)
{
    snprintf(out, size, "t%u", (unsigned int)index);
}

void load_tickers()
{
    uint32_t count = preferences.getUInt("ticker_count", 0);

    if (count > STORAGE_MAX_TICKERS) {
        Serial.printf(
            "[STORAGE] Aviso: contador invalido (%lu)\n",
            (unsigned long)count
        );
        count = 0;
    }

    Lock lock;

    ticker_count = 0;

    for (size_t i = 0; i < count; ++i) {
        char key[12];
        ticker_key(key, sizeof(key), i);

        String value = preferences.getString(key, "");

        if (value.isEmpty()) {
            continue;
        }

        tickers[ticker_count++] = value;
    }

    Serial.printf(
        "[STORAGE] %u tickers cargados\n",
        (unsigned int)ticker_count
    );
}

} // namespace

String storage_normalize_ticker(const String &input)
{
    String ticker = input;
    ticker.trim();
    ticker.toUpperCase();
    return ticker;
}

bool storage_ticker_format_valid(const String &input)
{
    String ticker = storage_normalize_ticker(input);

    if (ticker.isEmpty() || ticker.length() > 16) {
        return false;
    }

    bool has_alphanumeric = false;

    for (size_t i = 0; i < ticker.length(); ++i) {
        char c = ticker.charAt(i);

        bool alpha = c >= 'A' && c <= 'Z';
        bool digit = c >= '0' && c <= '9';

        if (alpha || digit) {
            has_alphanumeric = true;
        } else if (c != '.' && c != '-' && c != '^' && c != '=') {
            return false;
        }
    }

    return has_alphanumeric;
}

void storage_init()
{
    if (storage_ready) {
        return;
    }

    tickers_mutex = xSemaphoreCreateMutex();

    storage_ready = preferences.begin("crow_app", false);

    if (!storage_ready) {
        Serial.println(
            "[STORAGE] ERROR: No se pudo abrir Preferences. "
            "No se borraran los datos existentes."
        );
        return;
    }

    Serial.println("[STORAGE] Preferences abierto correctamente");

    load_tickers();

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    String ssid = preferences.getString("wifi_ssid", "");
    String password = preferences.getString("wifi_pass", "");

    if (!ssid.isEmpty()) {
        Serial.printf(
            "[WIFI] Iniciando conexion a red guardada: %s\n",
            ssid.c_str()
        );

        if (WiFi.status() != WL_CONNECTED) {
            WiFi.begin(ssid.c_str(), password.c_str());
        }
    }
}

size_t storage_ticker_count()
{
    Lock lock;
    return ticker_count;
}

String storage_get_ticker(size_t index)
{
    Lock lock;

    if (index >= ticker_count) {
        return String();
    }

    return tickers[index];
}

bool storage_ticker_exists(const String &input)
{
    String ticker = storage_normalize_ticker(input);

    Lock lock;

    for (size_t i = 0; i < ticker_count; ++i) {
        if (tickers[i] == ticker) {
            return true;
        }
    }

    return false;
}

bool storage_add_ticker(const String &input)
{
    if (!storage_ready) {
        Serial.println("[STORAGE] ERROR: almacenamiento no inicializado");
        return false;
    }

    String ticker = storage_normalize_ticker(input);

    if (!storage_ticker_format_valid(ticker)) {
        Serial.println("[STORAGE] ERROR: formato de ticker invalido");
        return false;
    }

    if (storage_ticker_exists(ticker)) {
        Serial.println("[STORAGE] ERROR: ticker duplicado");
        return false;
    }

    Lock lock;

    if (ticker_count >= STORAGE_MAX_TICKERS) {
        Serial.println("[STORAGE] ERROR: limite de tickers alcanzado");
        return false;
    }

    char key[12];
    ticker_key(key, sizeof(key), ticker_count);

    if (preferences.putString(key, ticker) == 0) {
        Serial.printf("[STORAGE] ERROR: fallo al escribir %s\n", key);
        return false;
    }

    // Actualizar el contador solo despues de escribir el ticker.
    if (preferences.putUInt(
            "ticker_count",
            static_cast<uint32_t>(ticker_count + 1)) == 0) {
        Serial.println("[STORAGE] ERROR: ticker escrito, pero fallo el contador");
        return false;
    }

    tickers[ticker_count++] = ticker;

    Serial.printf(
        "[STORAGE] Ticker '%s' guardado en indice %u\n",
        ticker.c_str(),
        (unsigned int)(ticker_count - 1)
    );

    return true;
}

bool storage_remove_ticker(size_t index)
{
    if (!storage_ready) {
        return false;
    }

    Lock lock;

    if (index >= ticker_count) {
        return false;
    }

    // Desplazar en RAM y reescribir solo las claves afectadas.
    for (size_t i = index; i + 1 < ticker_count; ++i) {
        tickers[i] = tickers[i + 1];

        char key[12];
        ticker_key(key, sizeof(key), i);

        if (preferences.putString(key, tickers[i]) == 0) {
            Serial.println("[STORAGE] Error reordenando tickers");
            return false;
        }
    }

    char last_key[12];
    ticker_key(last_key, sizeof(last_key), ticker_count - 1);
    preferences.remove(last_key);

    tickers[ticker_count - 1] = String();
    --ticker_count;

    if (preferences.putUInt(
            "ticker_count",
            static_cast<uint32_t>(ticker_count)) == 0) {
        Serial.println("[STORAGE] Error actualizando contador");
        return false;
    }

    Serial.printf(
        "[STORAGE] Ticker del indice %u eliminado\n",
        (unsigned int)index
    );

    return true;
}

bool storage_clear_tickers()
{
    if (!storage_ready) {
        return false;
    }

    Lock lock;

    for (size_t i = 0; i < ticker_count; ++i) {
        char key[12];
        ticker_key(key, sizeof(key), i);
        preferences.remove(key);
        tickers[i] = String();
    }

    ticker_count = 0;
    preferences.putUInt("ticker_count", 0);

    Serial.println("[STORAGE] Todos los tickers eliminados");
    return true;
}

void storage_save_wifi_credentials(
    const String &ssid,
    const String &password
)
{
    if (!storage_ready) {
        Serial.println("[STORAGE] ERROR: Preferences no esta abierto");
        return;
    }

    size_t ssid_bytes = preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_pass", password);

    if (ssid_bytes == 0) {
        Serial.println("[STORAGE] ERROR: no se pudieron guardar las credenciales WiFi");
        return;
    }

    Serial.printf("[STORAGE] Credenciales guardadas para: %s\n", ssid.c_str());
}

bool storage_get_wifi_credentials(String &ssid, String &password)
{
    if (!storage_ready) {
        return false;
    }

    ssid = preferences.getString("wifi_ssid", "");
    password = preferences.getString("wifi_pass", "");

    return !ssid.isEmpty();
}

void storage_forget_wifi()
{
    if (storage_ready) {
        preferences.remove("wifi_ssid");
        preferences.remove("wifi_pass");
    }

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_STA);

    Serial.println("[STORAGE] Red WiFi olvidada");
}
