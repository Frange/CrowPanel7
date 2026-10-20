#include "ui_manager.h"
#include "screens/screen_list.h"
#include "screens/screen_detail.h"
#include "screens/screen_add.h"

static lv_obj_t *current_screen = nullptr;

void ui_init()
{
    // Carga inicial de la pantalla principal de lista
    current_screen = screen_list_create();

    if (current_screen != nullptr) {
        lv_scr_load(current_screen);
    }
}

void ui_change_screen(ScreenState state)
{
    lv_obj_t *new_screen = nullptr;

    // Crear la pantalla destino según el estado solicitado
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
        default:
            return;
    }

    if (new_screen != nullptr) {
        // Transición fluida estilo Android Auto (Fade de 200ms) 
        // y borra automáticamente la pantalla anterior para liberar memoria.
        lv_scr_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
        current_screen = new_screen;
    }
}

void ui_update()
{
    // Espacio reservado para lógica de actualización periódica de la interfaz si se requiere.
}