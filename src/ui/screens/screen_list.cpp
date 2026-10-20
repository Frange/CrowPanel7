
#include <lvgl.h>
#include "screen_list.h"

lv_obj_t* screen_list_create()
{
    // Pantalla principal
    lv_obj_t *scr = lv_obj_create(NULL);

    // Fondo verde sólido
    lv_obj_set_style_bg_color(
        scr,
        lv_color_hex(0x00FF00),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        scr,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    // Eliminar borde y padding de la pantalla
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);

    // Evitar desplazamientos
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Círculo rojo de 200 x 200 píxeles
    lv_obj_t *circle = lv_obj_create(scr);

    lv_obj_set_size(circle, 200, 200);
    lv_obj_center(circle);

    // Convertir el objeto cuadrado en un círculo
    lv_obj_set_style_radius(
        circle,
        LV_RADIUS_CIRCLE,
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_color(
        circle,
        lv_color_hex(0xFF0000),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        circle,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    // Eliminar borde y padding del círculo
    lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(circle, 0, LV_PART_MAIN);

    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    return scr;
}
