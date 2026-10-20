#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <lvgl.h>

enum ScreenState {
    SCREEN_LIST,
    SCREEN_DETAIL,
    SCREEN_ADD,
    SCREEN_SETTINGS
};

void ui_init();
void ui_change_screen(ScreenState state);
void ui_update();

#endif