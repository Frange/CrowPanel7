#ifndef UI_KEYBOARD_H
#define UI_KEYBOARD_H

#include <lvgl.h>

// Crea un teclado LVGL oculto inicialmente y lo vincula al campo de texto.
// El diseño de cada pantalla debe mantener el campo activo por encima del teclado.
lv_obj_t *ui_keyboard_attach(
    lv_obj_t *screen,
    lv_obj_t *textarea,
    lv_keyboard_mode_t mode
);

#endif
