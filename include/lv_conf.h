#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_HOR_RES_MAX    800
#define LV_VER_RES_MAX    480

#define LV_COLOR_DEPTH    16

// El panel RGB de la CrowPanel espera RGB565 con los bytes intercambiados.
// Con esto LVGL ya dibuja en ese formato y el volcado es un simple memcpy.
#define LV_COLOR_16_SWAP  1

// Memoria de LVGL en el heap del ESP32 en vez de un pool estatico de 48 KB.
#define LV_MEM_CUSTOM          1
#define LV_MEM_CUSTOM_INCLUDE  <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC    malloc
#define LV_MEM_CUSTOM_FREE     free
#define LV_MEM_CUSTOM_REALLOC  realloc
#define LV_MEMCPY_MEMSET_STD   1

// Refresco y lectura del tactil.
#define LV_DISP_DEF_REFR_PERIOD   20
#define LV_INDEV_DEF_READ_PERIOD  10

// Reloj de LVGL tomado de millis(): el tiempo ya no depende de lo que
// tarde cada vuelta de loop() en dibujar.
#define LV_TICK_CUSTOM                1
#define LV_TICK_CUSTOM_INCLUDE        "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR  (millis())

#define LV_USE_LOG  0

// Tema oscuro: los botones no llevan sombra (lo mas caro de dibujar)
// y sin animaciones al cambiar de estado.
#define LV_THEME_DEFAULT_DARK             1
#define LV_THEME_DEFAULT_TRANSITION_TIME  0

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1

#endif
