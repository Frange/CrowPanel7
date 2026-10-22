#pragma once

#include <Arduino.h>
#include <lvgl.h>

namespace ScreenListStyles {

constexpr int SCREEN_WIDTH = 800;
constexpr int TOOLBAR_HEIGHT = 65;
constexpr int HEADER_WIDTH = 780;
constexpr int HEADER_HEIGHT = 34;
constexpr int ROW_WIDTH = 780;
constexpr int ROW_HEIGHT = 48;
constexpr int ROW_SPACING = 53;
constexpr int CONTENT_HEIGHT = 365;

constexpr uint32_t BG = 0x121212;
constexpr uint32_t TOOLBAR_BG = 0x1F1F1F;
constexpr uint32_t HEADER_BG = 0x303030;
constexpr uint32_t ROW_EVEN = 0x1E1E1E;
constexpr uint32_t ROW_ODD = 0x252525;
constexpr uint32_t TEXT = 0xFFFFFF;
constexpr uint32_t TEXT_SECONDARY = 0xCCCCCC;
constexpr uint32_t TEXT_MUTED = 0xAAAAAA;
constexpr uint32_t BUTTON_BG = 0x333333;
constexpr uint32_t ADD_BG = 0x007ACC;
constexpr uint32_t DELETE_BG = 0x7A2525;

void apply_screen(lv_obj_t *obj);
void apply_toolbar(lv_obj_t *obj);
void apply_header(lv_obj_t *obj);
void apply_row(lv_obj_t *obj, size_t index);
void apply_label(lv_obj_t *obj, uint32_t color);
void apply_button(lv_obj_t *obj, uint32_t color);

}