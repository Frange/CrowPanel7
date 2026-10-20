
#include <Arduino.h>

#include "display_config.h"
#include "ui/ui_manager.h"
#include "services/storage_service.h"
#include "services/market_service.h"

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println("\n[BOOT] Inicio");

    // 1. Inicializar pantalla, bus y tactil.
    display_init();

    // 2. Inicializar almacenamiento y WiFi.
    storage_init();

    // 3. Inicializar consultas de mercado en segundo plano.
    market_service_init();

    // 4. Crear la interfaz.
    ui_init();

    Serial.println("[BOOT] Inicializacion terminada");
}

void loop()
{
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
}
