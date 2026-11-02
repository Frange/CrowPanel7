#include <Arduino.h>
#include <lvgl.h>

#include "display_config.h"
#include "ui/ui_manager.h"
#include "services/settings_service.h"
#include "services/storage_service.h"
#include "services/market_service.h"

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println("\n[BOOT] Inicio");

    // 1. Ajustes guardados (brillo, intervalos...).
    settings_init();

    // 2. Pantalla, bus y tactil.
    display_init();

    // 3. Almacenamiento de tickers y WiFi.
    storage_init();

    // 4. Consultas de mercado en segundo plano.
    market_service_init();

    // 5. Interfaz.
    ui_init();

    Serial.println("[BOOT] Inicializacion terminada");
}

void loop()
{
    // lv_timer_handler devuelve cuanto falta para el siguiente timer.
    uint32_t wait_ms = lv_timer_handler();

    if (wait_ms > 10) {
        wait_ms = 10;
    } else if (wait_ms < 1) {
        wait_ms = 1;
    }

    delay(wait_ms);
}
