#include "screen_list_styles.h"

namespace ScreenListStyles {

void apply_screen(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(BG), 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void apply_toolbar(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(TOOLBAR_BG), 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 8, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void apply_header(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(HEADER_BG), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 5, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void apply_row(lv_obj_t *obj, size_t index)
{
    lv_obj_set_style_bg_color(
        obj,
        lv_color_hex(index % 2 == 0 ? ROW_EVEN : ROW_ODD),
        0
    );

    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 5, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void apply_label(lv_obj_t *obj, uint32_t color)
{
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
}

void apply_button(lv_obj_t *obj, uint32_t color)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
}

}