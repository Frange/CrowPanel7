
#ifndef UI_KEYBOARD_H
#define UI_KEYBOARD_H

#include <lvgl.h>

lv_obj_t *ui_keyboard_attach(
    lv_obj_t *screen,
    lv_obj_t *textarea,
    lv_keyboard_mode_t mode
);

#endif
