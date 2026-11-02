#pragma once

#include <lvgl.h>

typedef void (*ui_confirm_cb_t)(void *user_data);

// Cuadro de confirmacion modal con "Cancelar" y un boton de accion.
// Si destructive es true el boton de accion se pinta en rojo.
void ui_confirm(
    const char *title,
    const char *message,
    const char *action_label,
    bool destructive,
    ui_confirm_cb_t on_confirm,
    void *user_data
);
