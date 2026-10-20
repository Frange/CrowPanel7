#include <Arduino.h>
#include "display_config.h"
#include "ui/ui_manager.h"

void setup() {
    // ESTO ES OBLIGATORIO PARA VER LOS LOGS
    Serial.begin(115200);
    delay(500); // Pequeña pausa para estabilizar el puerto serie

    Serial.println("\n[BOOT] Inicio");

    // Inicializar pantalla y táctil
    display_init();

    // Inicializar interfaz gráfica (LVGL)
    ui_init();
    
    Serial.println("[BOOT] Inicializacion terminada");
}

void loop() {
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
}