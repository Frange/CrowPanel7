#pragma once

#include <Arduino.h>

struct AppSettings {
    uint8_t  brightness;       // 10..255
    uint16_t refresh_minutes;  // minutos entre actualizaciones (1..60)
    uint16_t dim_minutes;      // minutos de inactividad para atenuar (0 = nunca)
};

void settings_init();

const AppSettings &settings_get();

void settings_set_brightness(uint8_t value);
void settings_set_refresh_minutes(uint16_t minutes);
void settings_set_dim_minutes(uint16_t minutes);
