#include <Arduino.h>
#include <lvgl.h>

#include "screen_list_row.h"
#include "screen_list_internal.h"
#include "screen_list_styles.h"
#include "screen_list_events.h"

lv_obj_t *screen_list_header_create(lv_obj_t *parent)
{
    using namespace ScreenListStyles;

    lv_obj_t *header = lv_obj_create(parent);

    lv_obj_set_size(header, HEADER_WIDTH, HEADER_HEIGHT);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 70);

    apply_header(header);

    struct Column {
        const char *title;
        lv_coord_t x;
        lv_coord_t width;
        bool right;
    };

    const Column columns[] = {
        {"SIMBOLO",    8,   100, false},
        {"ACTUAL",     145, 110, false},
        {"APERTURA",   290, 110, false},
        {"CIERRE ANT.",435, 110, false},
        {"CAMBIO",     -48, 75,  true}
    };

    for (const auto &column : columns) {
        lv_obj_t *label = lv_label_create(header);

        lv_label_set_text(label, column.title);
        lv_obj_set_width(label, column.width);
        apply_label(label, TEXT);

        lv_obj_align(
            label,
            column.right ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID,
            column.x,
            0
        );
    }

    return header;
}

lv_obj_t *screen_list_row_create(
    lv_obj_t *parent,
    size_t index,
    const String &symbol
)
{
    using namespace ScreenListStyles;

    if (index >= MAX_ROWS) {
        return nullptr;
    }

    auto &state = screen_list_state;

    lv_obj_t *row = lv_obj_create(parent);

    lv_obj_set_size(row, ROW_WIDTH, ROW_HEIGHT);

    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(
        row,
        screen_list_ticker_row_event_cb,
        LV_EVENT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(index))
    );

    lv_obj_align(
        row,
        LV_ALIGN_TOP_LEFT,
        0,
        static_cast<lv_coord_t>(index * ROW_SPACING)
    );

    apply_row(row, index);

    const struct {
        lv_obj_t **target;
        const char *text;
        lv_coord_t x;
        lv_coord_t width;
        uint32_t color;
        bool right;
    } columns[] = {
        {&state.ticker_labels[index], symbol.c_str(), 8, 100, TEXT, false},
        {&state.price_labels[index], "--", 145, 110, TEXT, false},
        {&state.open_labels[index], "--", 290, 110, TEXT_SECONDARY, false},
        {&state.close_labels[index], "--", 435, 110, TEXT_SECONDARY, false},
        {&state.change_labels[index], "--", -48, 75, TEXT_MUTED, true}
    };

    for (const auto &column : columns) {
        *column.target = lv_label_create(row);

        lv_label_set_text(*column.target, column.text);
        lv_obj_set_width(*column.target, column.width);
        apply_label(*column.target, column.color);

        lv_obj_align(
            *column.target,
            column.right ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID,
            column.x,
            0
        );
    }

    state.delete_buttons[index] = lv_btn_create(row);

    lv_obj_set_size(state.delete_buttons[index], 32, 32);
    lv_obj_align(state.delete_buttons[index], LV_ALIGN_RIGHT_MID, -4, 0);

    apply_button(state.delete_buttons[index], DELETE_BG);

    lv_obj_set_style_radius(state.delete_buttons[index], 6, 0);

    lv_obj_add_event_cb(
        state.delete_buttons[index],
        screen_list_delete_btn_event_cb,
        LV_EVENT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(index))
    );

    lv_obj_t *icon = lv_label_create(state.delete_buttons[index]);
    lv_label_set_text(icon, LV_SYMBOL_TRASH);
    lv_obj_center(icon);

    return row;
}