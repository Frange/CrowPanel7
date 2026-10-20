
#include "ui_manager.h"
#include "screens/screen_list.h"

static lv_obj_t *current_screen = nullptr;

void ui_init()
{
    current_screen = screen_list_create();

    if (current_screen != nullptr) {
        lv_scr_load(current_screen);
    }
}

void ui_change_screen(ScreenState state)
{
    // En esta prueba solo existe SCREEN_LIST.
    if (state != SCREEN_LIST) {
        return;
    }

    if (current_screen != nullptr) {
        lv_scr_load(current_screen);
    }
}

void ui_update()
{
    // El loop() de main.cpp ya procesa LVGL.
}
