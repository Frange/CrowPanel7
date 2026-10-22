
#include <lvgl.h>

#include "screen_list_toolbar.h"
#include "screen_list_styles.h"
#include "screen_list_internal.h"
#include "screen_list_events.h"

#include "../../ui_manager.h"

static void wifi_button_event_cb(lv_event_t *e)
{
    (void)e;
    ui_change_screen(SCREEN_WIFI);
}

lv_obj_t *screen_list_toolbar_create(lv_obj_t *parent)
{
    using namespace ScreenListStyles;

    lv_obj_t *toolbar = lv_obj_create(parent);

    lv_obj_set_size(toolbar, SCREEN_WIDTH, TOOLBAR_HEIGHT);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    apply_toolbar(toolbar);

    lv_obj_t *title = lv_label_create(toolbar);
    lv_label_set_text(title, "Mi Portafolio");
    apply_label(title, TEXT);

    lv_obj_set_style_text_font(
        title, &lv_font_montserrat_14, 0
    );
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    auto create_button = [toolbar](
        const char *symbol,
        int x_offset,
        uint32_t color,
        lv_event_cb_t callback
    ) {
        lv_obj_t *button = lv_btn_create(toolbar);

        lv_obj_set_size(button, 48, 48);
        lv_obj_align(
            button, LV_ALIGN_RIGHT_MID, x_offset, 0
        );
        apply_button(button, color);

        lv_obj_add_event_cb(
            button,
            callback,
            LV_EVENT_CLICKED,
            nullptr
        );

        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, symbol);
        lv_obj_center(label);

        return button;
    };

    // Botón WiFi: conserva la etiqueta que actualiza
    // screen_list_update_wifi_indicator().
    lv_obj_t *wifi_button = lv_btn_create(toolbar);

    lv_obj_set_size(wifi_button, 140, 44);
    lv_obj_align(
        wifi_button,
        LV_ALIGN_RIGHT_MID,
        -178,
        0
    );

    apply_button(wifi_button, BUTTON_BG);

    lv_obj_add_event_cb(
        wifi_button,
        wifi_button_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    screen_list_state.wifi_indicator =
        lv_label_create(wifi_button);

    lv_obj_set_width(
        screen_list_state.wifi_indicator, 140
    );

    lv_obj_set_style_text_font(
        screen_list_state.wifi_indicator,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_align(
        screen_list_state.wifi_indicator,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_center(screen_list_state.wifi_indicator);

    create_button(
        LV_SYMBOL_SETTINGS,
        -8,
        BUTTON_BG,
        screen_list_settings_btn_event_cb
    );

    create_button(
        LV_SYMBOL_PLUS,
        -66,
        ADD_BG,
        screen_list_add_btn_event_cb
    );

    create_button(
        LV_SYMBOL_REFRESH,
        -124,
        BUTTON_BG,
        screen_list_refresh_btn_event_cb
    );

    return toolbar;
}
