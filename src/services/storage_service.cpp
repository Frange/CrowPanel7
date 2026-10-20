
#include "storage_service.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>

static Preferences preferences;
static bool storage_ready = false;

static String normalize_ticker(const String &input)
{
    String ticker = input;
    ticker.trim();
    ticker.toUpperCase();
    return ticker;
}

static bool valid_ticker_format(const String &input)
{
    String ticker = normalize_ticker(input);

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

    storage_ready = preferences.begin("crow_app", false);

    if (!storage_ready) {
        Serial.println(
            "[STORAGE] ERROR: No se pudo abrir Preferences. "
            "No se borraran los datos existentes."
        );
        return;
    }

    Serial.println("[STORAGE] Preferences abierto correctamente");

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
    if (!storage_ready) {
        return 0;
    }

    uint32_t count = preferences.getUInt("ticker_count", 0);

    if (count > 40) {
        Serial.printf(
            "[STORAGE] Aviso: contador invalido (%lu)\n",
            (unsigned long)count
        );
        return 0;
    }

    return static_cast<size_t>(count);
}

String storage_get_ticker(size_t index)
{
    if (!storage_ready || index >= storage_ticker_count()) {
        return String();
    }

    char key[8];
    snprintf(key, sizeof(key), "t%u", (unsigned int)index);

    return preferences.getString(key, "");
}

bool storage_ticker_exists(const String &input)
{
    if (!storage_ready) {
        return false;
    }

    String ticker = normalize_ticker(input);

    for (size_t i = 0; i < storage_ticker_count(); ++i) {
        if (storage_get_ticker(i) == ticker) {
            return true;
        }
    }

    return false;
}

bool storage_add_ticker(const String &input)
{
    if (!storage_ready) {
        Serial.println(
            "[STORAGE] ERROR: almacenamiento no inicializado"
        );
        return false;
    }

    String ticker = normalize_ticker(input);

    if (!valid_ticker_format(ticker)) {
        Serial.println("[STORAGE] ERROR: formato de ticker invalido");
        return false;
    }

    if (storage_ticker_exists(ticker)) {
        Serial.println("[STORAGE] ERROR: ticker duplicado");
        return false;
    }

    size_t count = storage_ticker_count();

    if (count >= 40) {
        Serial.println("[STORAGE] ERROR: limite de 40 tickers alcanzado");
        return false;
    }

    char key[8];
    snprintf(key, sizeof(key), "t%u", (unsigned int)count);

    size_t bytes_written = preferences.putString(key, ticker);

    if (bytes_written == 0) {
        Serial.printf(
            "[STORAGE] ERROR: fallo al escribir la clave %s\n",
            key
        );
        return false;
    }

    // Actualizar el contador únicamente después de escribir el ticker.
    size_t counter_bytes = preferences.putUInt(
        "ticker_count",
        static_cast<uint32_t>(count + 1)
    );

    if (counter_bytes == 0) {
        Serial.println(
            "[STORAGE] ERROR: ticker escrito, pero fallo el contador"
        );
        return false;
    }

    Serial.printf(
        "[STORAGE] Ticker '%s' guardado en indice %u\n",
        ticker.c_str(),
        (unsigned int)count
    );

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
    size_t pass_bytes = preferences.putString("wifi_pass", password);

    if (ssid_bytes == 0 || pass_bytes == 0) {
        Serial.println(
            "[STORAGE] ERROR: no se pudieron guardar las credenciales WiFi"
        );
        return;
    }

    Serial.printf(
        "[STORAGE] Credenciales guardadas para: %s\n",
        ssid.c_str()
    );
}

bool storage_get_wifi_credentials(
    String &ssid,
    String &password
)
{
    if (!storage_ready) {
        return false;
    }

    ssid = preferences.getString("wifi_ssid", "");
    password = preferences.getString("wifi_pass", "");

    return !ssid.isEmpty();
}


bool storage_remove_ticker(size_t index)
{
    if (!storage_ready) {
        return false;
    }

    size_t count = storage_ticker_count();

    if (index >= count) {
        return false;
    }

    for (size_t i = index; i + 1 < count; ++i) {
        char current_key[8];
        char next_key[8];

        snprintf(current_key, sizeof(current_key), "t%u",
                 (unsigned int)i);
        snprintf(next_key, sizeof(next_key), "t%u",
                 (unsigned int)(i + 1));

        String next = preferences.getString(next_key, "");

        if (next.isEmpty() ||
            preferences.putString(current_key, next) == 0) {
            Serial.println("[STORAGE] Error reordenando tickers");
            return false;
        }
    }

    char last_key[8];
    snprintf(last_key, sizeof(last_key), "t%u",
             (unsigned int)(count - 1));

    preferences.remove(last_key);

    if (preferences.putUInt("ticker_count", count - 1) == 0) {
        Serial.println("[STORAGE] Error actualizando contador");
        return false;
    }

    Serial.printf("[STORAGE] Ticker del indice %u eliminado\n",
                  (unsigned int)index);

    return true;
}

int storage_wifi_status()
{
    if (WiFi.status() != WL_CONNECTED) {
        return STORAGE_WIFI_DISCONNECTED;
    }

    // La conexion Wi-Fi esta activa. No garantiza por si sola
    // que haya acceso a Internet.
    return STORAGE_WIFI_INTERNET;
}

