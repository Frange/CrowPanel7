#define LGFX_USE_V1

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lvgl.h>
#include <esp_heap_caps.h>

#include "display_config.h"

class LGFX : public lgfx::LGFX_Device {
public:
    lgfx::Panel_RGB _panel_instance;
    lgfx::Bus_RGB _bus_instance;
    lgfx::Touch_GT911 _touch_instance;

    LGFX() {
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;

            // Azul
            cfg.pin_d0 = GPIO_NUM_15;
            cfg.pin_d1 = GPIO_NUM_7;
            cfg.pin_d2 = GPIO_NUM_6;
            cfg.pin_d3 = GPIO_NUM_5;
            cfg.pin_d4 = GPIO_NUM_4;

            // Verde
            cfg.pin_d5  = GPIO_NUM_9;
            cfg.pin_d6  = GPIO_NUM_46;
            cfg.pin_d7  = GPIO_NUM_3;
            cfg.pin_d8  = GPIO_NUM_8;
            cfg.pin_d9  = GPIO_NUM_16;
            cfg.pin_d10 = GPIO_NUM_1;

            // Rojo
            cfg.pin_d11 = GPIO_NUM_14;
            cfg.pin_d12 = GPIO_NUM_21;
            cfg.pin_d13 = GPIO_NUM_47;
            cfg.pin_d14 = GPIO_NUM_48;
            cfg.pin_d15 = GPIO_NUM_45;

            // Señales RGB (Pines originales estables)
            cfg.pin_henable = GPIO_NUM_41;
            cfg.pin_vsync   = GPIO_NUM_40;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_0;

            cfg.freq_write = 15000000;

            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 40;
            cfg.hsync_pulse_width = 48;
            cfg.hsync_back_porch  = 40;

            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 1;
            cfg.vsync_pulse_width = 31;
            cfg.vsync_back_porch  = 13;

            cfg.pclk_active_neg = 1;
            cfg.de_idle_high    = 0;
            cfg.pclk_idle_high  = 0;

            _bus_instance.config(cfg);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = SCREEN_WIDTH;
            cfg.memory_height = SCREEN_HEIGHT;
            cfg.panel_width   = SCREEN_WIDTH;
            cfg.panel_height  = SCREEN_HEIGHT;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }

        _panel_instance.setBus(&_bus_instance);

        {
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.x_max = SCREEN_WIDTH - 1;
            cfg.y_min = 0;
            cfg.y_max = SCREEN_HEIGHT - 1;
            cfg.pin_int = GPIO_NUM_NC;
            cfg.pin_rst = GPIO_NUM_NC;
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;
            cfg.i2c_port = I2C_NUM_1;
            cfg.pin_sda = GPIO_NUM_19;
            cfg.pin_scl = GPIO_NUM_20;
            cfg.freq = 400000;

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};

static LGFX lcd;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;

static void disp_flush(
    lv_disp_drv_t *disp,
    const lv_area_t *area,
    lv_color_t *color_p
) {
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;

    lcd.pushImage(
        area->x1,
        area->y1,
        w,
        h,
        reinterpret_cast<const lgfx::rgb565_t *>(color_p)
    );

    lv_disp_flush_ready(disp);
}

static void touchpad_read(
    lv_indev_drv_t *indev_driver,
    lv_indev_data_t *data
) {
    (void)indev_driver;

    int16_t x = 0;
    int16_t y = 0;
    bool touched = false;

    display_get_touch(&x, &y, &touched);

    if (touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void display_init() {
    Serial.println("[LCD] Iniciando LovyanGFX...");

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);

    lcd.init();
    lcd.setBrightness(255);

    Serial.println("[LCD] LovyanGFX inicializado");

    lv_init();

    constexpr size_t buffer_pixels = static_cast<size_t>(SCREEN_WIDTH) * 40;
    constexpr size_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);

    buf1 = static_cast<lv_color_t *>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    buf2 = static_cast<lv_color_t *>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (buf1 == nullptr || buf2 == nullptr) {
        Serial.println("[LCD] ERROR: memoria insuficiente para LVGL");
        return;
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    Serial.println("[LCD] Display y Tactil LVGL registrados");
}

void display_get_touch(
    int16_t *x,
    int16_t *y,
    bool *touched
) {
    if (x == nullptr || y == nullptr || touched == nullptr) {
        return;
    }

    lgfx::touch_point_t tp;
    const int count = lcd.getTouch(&tp, 1);

    if (count > 0) {
        *x = tp.x;
        *y = tp.y;
        *touched = true;
    } else {
        *touched = false;
    }
}