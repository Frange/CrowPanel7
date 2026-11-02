#include "ui_dialog.h"

namespace {

const char *button_texts[3] = {"Cancelar", "", ""};

ui_confirm_cb_t pending_cb = nullptr;
void *pending_user_data = nullptr;
lv_obj_t *active_box = nullptr;

void msgbox_event_cb(lv_event_t *e)
{
    lv_obj_t *box = lv_event_get_current_target(e);

    if (box != active_box) {
        return;
    }

    uint16_t index = lv_msgbox_get_active_btn(box);

    ui_confirm_cb_t cb = pending_cb;
    void *user_data = pending_user_data;

    pending_cb = nullptr;
    pending_user_data = nullptr;
    active_box = nullptr;

    lv_msgbox_close(box);

    if (index == 1 && cb != nullptr) {
        cb(user_data);
    }
}

// Pinta en rojo el boton de accion (indice 1) de la matriz de botones.
void destructive_draw_cb(lv_event_t *e)
{
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);

    if (dsc->class_p == &lv_btnmatrix_class &&
        dsc->type == LV_BTNMATRIX_DRAW_PART_BTN &&
        dsc->id == 1 &&
        dsc->rect_dsc != nullptr) {
        dsc->rect_dsc->bg_color = lv_color_hex(0xB03030);
    }
}

} // namespace

void ui_confirm(
    const char *title,
    const char *message,
    const char *action_label,
    bool destructive,
    ui_confirm_cb_t on_confirm,
    void *user_data
)
{
    if (active_box != nullptr) {
        return;
    }

    button_texts[1] = action_label;

    pending_cb = on_confirm;
    pending_user_data = user_data;

    // Padre nulo: modal sobre la capa superior, con fondo oscurecido.
    active_box = lv_msgbox_create(
        nullptr, title, message, button_texts, false
    );

    lv_obj_set_width(active_box, 460);
    lv_obj_center(active_box);

    lv_obj_t *buttons = lv_msgbox_get_btns(active_box);

    lv_obj_set_width(buttons, 300);
    lv_obj_set_height(buttons, 44);
    lv_obj_set_style_pad_gap(buttons, 12, 0);

    if (destructive) {
        lv_obj_add_event_cb(
            buttons,
            destructive_draw_cb,
            LV_EVENT_DRAW_PART_BEGIN,
            nullptr
        );
    }

    lv_obj_add_event_cb(
        active_box,
        msgbox_event_cb,
        LV_EVENT_VALUE_CHANGED,
        nullptr
    );
}
