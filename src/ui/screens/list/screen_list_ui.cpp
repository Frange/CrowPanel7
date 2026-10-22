#include <Arduino.h>
#include <lvgl.h>

#include "screen_list_internal.h"
#include "screen_list_ui.h"
#include "screen_list_styles.h"
#include "screen_list_toolbar.h"
#include "screen_list_row.h"

#include "../../../services/storage_service.h"

void screen_list_build_ui()
{
    using namespace ScreenListStyles;

    auto &state = screen_list_state;
    lv_obj_t *screen = state.screen;

    if (screen == nullptr) {
        return;
    }

    apply_screen(screen);

    screen_list_toolbar_create(screen);
    screen_list_header_create(screen);

    state.list_content = lv_obj_create(screen);

    lv_obj_set_size(
        state.list_content,
        HEADER_WIDTH,
        CONTENT_HEIGHT
    );

    lv_obj_align(
        state.list_content,
        LV_ALIGN_TOP_MID,
        0,
        108
    );

    lv_obj_set_style_bg_color(
        state.list_content,
        lv_color_hex(BG),
        0
    );

    lv_obj_set_style_border_width(state.list_content, 0, 0);
    lv_obj_set_style_pad_all(state.list_content, 0, 0);
    lv_obj_set_scroll_dir(state.list_content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(
        state.list_content,
        LV_SCROLLBAR_MODE_AUTO
    );

    state.empty_label = lv_label_create(state.list_content);

    lv_label_set_text(
        state.empty_label,
        "No hay acciones ni ETFs.\n"
        "Pulsa + para anadir tu primer ticker."
    );

    apply_label(state.empty_label, TEXT_MUTED);

    lv_obj_align(
        state.empty_label,
        LV_ALIGN_TOP_MID,
        0,
        40
    );

    size_t count = storage_ticker_count();

    if (count > MAX_ROWS) {
        count = MAX_ROWS;
    }

    for (size_t i = 0; i < count; ++i) {
        String symbol = storage_get_ticker(i);

        if (!symbol.isEmpty()) {
            screen_list_row_create(
                state.list_content,
                i,
                symbol
            );
        }
    }

    state.refresh_status = lv_label_create(screen);

    apply_label(state.refresh_status, TEXT_MUTED);

    lv_obj_align(
        state.refresh_status,
        LV_ALIGN_BOTTOM_LEFT,
        15,
        -7
    );

    screen_list_update_rows();
    screen_list_update_wifi_indicator();
}