#include "settings_service.h"

#include <Preferences.h>

namespace {

constexpr uint8_t  DEFAULT_BRIGHTNESS = 255;
constexpr uint16_t DEFAULT_REFRESH_MIN = 5;
constexpr uint16_t DEFAULT_DIM_MIN = 0;

constexpr uint8_t  MIN_BRIGHTNESS = 10;

Preferences prefs;
bool ready = false;

AppSettings settings = {
    DEFAULT_BRIGHTNESS,
    DEFAULT_REFRESH_MIN,
    DEFAULT_DIM_MIN
};

} // namespace

void settings_init()
{
    if (ready) {
        return;
    }

    ready = prefs.begin("crow_cfg", false);

    if (!ready) {
        Serial.println("[SETTINGS] No se pudo abrir Preferences; usando valores por defecto");
        return;
    }

    settings.brightness =
        prefs.getUChar("bright", DEFAULT_BRIGHTNESS);
    settings.refresh_minutes =
        prefs.getUShort("refresh", DEFAULT_REFRESH_MIN);
    settings.dim_minutes =
        prefs.getUShort("dim", DEFAULT_DIM_MIN);

    if (settings.brightness < MIN_BRIGHTNESS) {
        settings.brightness = MIN_BRIGHTNESS;
    }

    if (settings.refresh_minutes < 1 || settings.refresh_minutes > 60) {
        settings.refresh_minutes = DEFAULT_REFRESH_MIN;
    }

    Serial.printf(
        "[SETTINGS] brillo=%u refresco=%u min atenuar=%u min\n",
        settings.brightness,
        settings.refresh_minutes,
        settings.dim_minutes
    );
}

const AppSettings &settings_get()
{
    return settings;
}

void settings_set_brightness(uint8_t value)
{
    if (value < MIN_BRIGHTNESS) {
        value = MIN_BRIGHTNESS;
    }

    settings.brightness = value;

    if (ready) {
        prefs.putUChar("bright", value);
    }
}

void settings_set_refresh_minutes(uint16_t minutes)
{
    if (minutes < 1) {
        minutes = 1;
    } else if (minutes > 60) {
        minutes = 60;
    }

    settings.refresh_minutes = minutes;

    if (ready) {
        prefs.putUShort("refresh", minutes);
    }
}

void settings_set_dim_minutes(uint16_t minutes)
{
    settings.dim_minutes = minutes;

    if (ready) {
        prefs.putUShort("dim", minutes);
    }
}
