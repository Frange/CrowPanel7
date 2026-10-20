#include <Arduino.h>
#include "display_config.h"
#include "ui/ui_manager.h"
#include "services/storage_service.h"

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println("\n[BOOT] Inicio");

    // 1. Inicializar pantalla, bus I2C y táctil.
    display_init();

    // 2. Abrir Preferences antes de utilizar cualquier función
    //    del servicio de almacenamiento.
    storage_init();

    // 3. Crear las pantallas de LVGL.
    ui_init();

    Serial.println("[BOOT] Inicializacion terminada");
}

void loop()
{
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
}