
#include "ui_manager.h"
#include "screens/list/screen_list.h"
#include "screens/detail/screen_detail.h"
#include "screens/add/screen_add.h"
#include "screens/settings/screen_settings.h"

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
    lv_obj_t *new_screen = nullptr;

    switch (state) {
        case SCREEN_LIST:
            new_screen = screen_list_create();
            break;

        case SCREEN_DETAIL:
            new_screen = screen_detail_create();
            break;

        case SCREEN_ADD:
            new_screen = screen_add_create();
            break;

        case SCREEN_SETTINGS:
            new_screen = screen_settings_create();
            break;

        default:
            return;
    }

    if (new_screen == nullptr) {
        return;
    }

    lv_obj_t *old_screen = current_screen;

    // Cambiar inmediatamente, sin fundido.
    lv_scr_load(new_screen);
    current_screen = new_screen;

    // Eliminar la pantalla anterior de forma segura.
    if (old_screen != nullptr && old_screen != new_screen) {
        lv_obj_del_async(old_screen);
    }
}

void ui_update()
{
}
