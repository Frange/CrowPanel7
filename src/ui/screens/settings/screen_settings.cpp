
#include <lvgl.h>

#include "screen_settings.h"
#include "../../ui_manager.h"

static void settings_back_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_LIST);
}

lv_obj_t *screen_settings_create()
{
    lv_obj_t *screen = lv_obj_create(nullptr);

    lv_obj_set_style_bg_color(
        screen, lv_color_hex(0x121212), 0
    );
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Barra superior
    lv_obj_t *top_bar = lv_obj_create(screen);

    lv_obj_set_size(top_bar, 800, 65);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_set_style_bg_color(
        top_bar, lv_color_hex(0x1F1F1F), 0
    );
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Botón volver
    lv_obj_t *back_button = lv_btn_create(top_bar);

    lv_obj_set_size(back_button, 100, 42);
    lv_obj_align(
        back_button, LV_ALIGN_LEFT_MID, 15, 0
    );

    lv_obj_set_style_bg_color(
        back_button, lv_color_hex(0x333333), 0
    );
    lv_obj_set_style_radius(back_button, 8, 0);

    lv_obj_add_event_cb(
        back_button,
        settings_back_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "< Volver");
    lv_obj_center(back_label);

    // Título
    lv_obj_t *title = lv_label_create(top_bar);

    lv_label_set_text(title, "Ajustes");
    lv_obj_set_style_text_color(
        title, lv_color_hex(0xFFFFFF), 0
    );
    lv_obj_set_style_text_font(
        title, &lv_font_montserrat_14, 0
    );
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Contenido inicial
    lv_obj_t *message = lv_label_create(screen);

    lv_label_set_text(
        message,
        "Configuracion general\n\n"
        "Aqui se anadiran las opciones de la aplicacion."
    );

    lv_obj_set_style_text_color(
        message, lv_color_hex(0xCCCCCC), 0
    );
    lv_obj_set_style_text_align(
        message, LV_TEXT_ALIGN_CENTER, 0
    );

    lv_obj_align(message, LV_ALIGN_CENTER, 0, 20);

    return screen;
}
