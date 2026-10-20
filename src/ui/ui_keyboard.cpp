
#include "ui_keyboard.h"

static void keyboard_textarea_event_cb(lv_event_t *e)
{
    lv_obj_t *keyboard = (lv_obj_t *)lv_event_get_user_data(e);

    if (keyboard == nullptr) {
        return;
    }

    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(keyboard);
    }
}

static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code != LV_EVENT_READY && code != LV_EVENT_CANCEL) {
        return;
    }

    lv_obj_t *keyboard = lv_event_get_target(e);

    if (keyboard != nullptr) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_keyboard_attach(
    lv_obj_t *screen,
    lv_obj_t *textarea,
    lv_keyboard_mode_t mode
)
{
    if (screen == nullptr || textarea == nullptr) {
        return nullptr;
    }

    lv_obj_t *keyboard = lv_keyboard_create(screen);

    lv_obj_set_size(keyboard, 800, 220);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_keyboard_set_mode(keyboard, mode);
    lv_keyboard_set_textarea(keyboard, textarea);

    lv_obj_add_event_cb(
        keyboard,
        keyboard_event_cb,
        LV_EVENT_ALL,
        nullptr
    );

    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(
        textarea,
        keyboard_textarea_event_cb,
        LV_EVENT_CLICKED,
        keyboard
    );

    return keyboard;
}
