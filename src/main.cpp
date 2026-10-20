
#include <Arduino.h>
#include <lvgl.h>

#include "display_config.h"
#include "ui/ui_manager.h"
#include "services/storage_service.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("[BOOT] Inicio");

    Serial.printf(
        "[MEM] PSRAM detectada: %s\n",
        psramFound() ? "SI" : "NO"
    );

    Serial.printf(
        "[MEM] PSRAM: %u bytes\n",
        (unsigned int)ESP.getPsramSize()
    );

    Serial.printf(
        "[MEM] Flash: %u bytes\n",
        (unsigned int)ESP.getFlashChipSize()
    );

    Serial.println("[1] Inicializando almacenamiento...");
    storage_init();
    Serial.println("[1] Almacenamiento OK");

    Serial.println("[2] Inicializando pantalla...");
    display_init();
    Serial.println("[2] Inicializacion de pantalla finalizada");

    Serial.println("[3] Inicializando interfaz...");
    ui_init();
    Serial.println("[3] Interfaz inicializada");

    Serial.println("[BOOT] Inicializacion terminada");
}

void loop()
{
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
}
