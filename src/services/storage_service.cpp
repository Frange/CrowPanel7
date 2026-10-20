#include "storage_service.h"

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

    if (ticker.length() == 0 || ticker.length() > 16) {
        return false;
    }

    bool has_alphanumeric = false;

    for (size_t i = 0; i < ticker.length(); ++i) {
        char c = ticker.charAt(i);

        const bool alpha = (c >= 'A' && c <= 'Z');
        const bool digit = (c >= '0' && c <= '9');

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
    if (!storage_ready) {
        storage_ready = preferences.begin("crow_app", false);
    }

    if (!storage_ready) {
        Serial.println("[STORAGE] No se pudo abrir Preferences");
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    String ssid = preferences.getString("wifi_ssid", "");
    String password = preferences.getString("wifi_pass", "");

    if (ssid.length() > 0) {
        Serial.printf("[WIFI] Intentando reconectar a la red guardada: %s\n",
                      ssid.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
    }
}

size_t storage_ticker_count()
{
    if (!storage_ready) return 0;

    uint32_t count = preferences.getUInt("ticker_count", 0);
    if (count > 40) count = 40;
    return (size_t)count;
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
    if (!storage_ready) return false;

    String ticker = normalize_ticker(input);

    if (!valid_ticker_format(ticker) ||
        storage_ticker_exists(ticker) ||
        storage_ticker_count() >= 40) {
        return false;
    }

    size_t count = storage_ticker_count();
    char key[8];
    snprintf(key, sizeof(key), "t%u", (unsigned int)count);

    if (preferences.putString(key, ticker) == 0) {
        return false;
    }

    preferences.putUInt("ticker_count", (uint32_t)(count + 1));
    return true;
}

void storage_save_wifi_credentials(const String &ssid, const String &password)
{
    if (!storage_ready) return;

    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_pass", password);
}

bool storage_get_wifi_credentials(String &ssid, String &password)
{
    if (!storage_ready) return false;

    ssid = preferences.getString("wifi_ssid", "");
    password = preferences.getString("wifi_pass", "");

    return ssid.length() > 0;
}

int storage_wifi_status()
{
    if (WiFi.status() != WL_CONNECTED) {
        return STORAGE_WIFI_DISCONNECTED;
    }

    // Una conexión TCP a una IP pública comprueba que hay salida a Internet
    // sin depender de que funcione la resolución DNS.
    WiFiClient client;
    client.setTimeout(800);

    IPAddress probe(1, 1, 1, 1);
    bool internet_ok = client.connect(probe, 443, 800);
    client.stop();

    return internet_ok ? STORAGE_WIFI_INTERNET : STORAGE_WIFI_NO_INTERNET;
}
