#define LGFX_USE_V1

#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lvgl.h>
#include <esp_heap_caps.h>

#include "display_config.h"

class LGFX : public lgfx::LGFX_Device {
public:
    lgfx::Panel_RGB _panel_instance;
    lgfx::Bus_RGB   _bus_instance;

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

            // Señales RGB
            cfg.pin_henable = GPIO_NUM_41;
            cfg.pin_vsync   = GPIO_NUM_40;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_0;

            cfg.freq_write = 15000000;   // si baila, prueba 12000000

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
        setPanel(&_panel_instance);
    }
};

static LGFX lcd;

// ---------------------------------------------------------------------------
// GT911 (lectura directa por I2C)
// ---------------------------------------------------------------------------
static uint8_t gt_addr = 0x5D;
static bool    gt_ok   = false;

static bool gt_read(uint16_t reg, uint8_t *buf, size_t len) {
    Wire.beginTransmission(gt_addr);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    if (Wire.endTransmission(true) != 0) return false;
    if (Wire.requestFrom((int)gt_addr, (int)len) != (int)len) return false;
    for (size_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

static void gt_write8(uint16_t reg, uint8_t v) {
    Wire.beginTransmission(gt_addr);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    Wire.write(v);
    Wire.endTransmission(true);
}

static bool gt_init() {
    for (uint8_t a : {(uint8_t)0x5D, (uint8_t)0x14}) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            gt_addr = a;
            uint8_t id[4] = {0};
            gt_read(0x8140, id, 4);
            Serial.printf("[TOUCH] GT911 en 0x%02X, ID=%c%c%c%c\n",
                          a, id[0], id[1], id[2], id[3]);
            gt_write8(0x814E, 0);   // limpiar flag inicial
            return true;
        }
    }
    Serial.println("[TOUCH] GT911 NO responde en 0x5D ni 0x14");
    return false;
}

// ---------------------------------------------------------------------------
// LVGL
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void display_init() {
    Serial.println("[LCD] Iniciando I2C y Pantalla...");

    Wire.begin(19, 20);
    Wire.setClock(400000);
    Wire.setTimeOut(50);

    // PCA9557 de la CrowPanel V3.0 (controla el reset del GT911)
    constexpr uint8_t PCA9557_ADDR = 0x18;

    auto pca_write = [](uint8_t reg, uint8_t value) -> bool {
        Wire.beginTransmission(PCA9557_ADDR);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    };

    Serial.println("[TOUCH] Inicializando PCA9557...");

    if (!pca_write(0x03, 0x00)) {            // todos como salida
        Serial.println("[TOUCH] ERROR configurando PCA9557");
    } else {
        pca_write(0x01, 0x00);               // reset GT911 activo
        delay(20);
        pca_write(0x01, 0x01);               // liberar reset
        delay(100);

        if (!pca_write(0x03, 0x02)) {        // IO0 salida, IO1 entrada (INT)
            Serial.println("[TOUCH] ERROR configurando INT");
        } else {
            Serial.println("[TOUCH] PCA9557 inicializado");
        }
    }

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);                   // backlight

    lcd.init();
    lcd.setBrightness(255);
    Serial.println("[LCD] LovyanGFX inicializado");

    Serial.println("[TOUCH] Inicializando GT911...");
    gt_ok = gt_init();
    Serial.println("[TOUCH] Inicializacion terminada");

    lv_init();

    // Buffers LVGL en RAM interna (evita competir con el framebuffer en PSRAM)
    constexpr size_t buffer_pixels = static_cast<size_t>(SCREEN_WIDTH) * 20;
    constexpr size_t buffer_bytes  = buffer_pixels * sizeof(lv_color_t);

    buf1 = static_cast<lv_color_t *>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    buf2 = static_cast<lv_color_t *>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

    if (buf1 == nullptr || buf2 == nullptr) {
        Serial.println("[LCD] ERROR: memoria insuficiente para LVGL");
        return;
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = SCREEN_WIDTH;
    disp_drv.ver_res  = SCREEN_HEIGHT;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    Serial.println("[LCD] Display y Tactil LVGL registrados");
}

// ---------------------------------------------------------------------------
// Touch
// ---------------------------------------------------------------------------
void display_get_touch(
    int16_t *x,
    int16_t *y,
    bool *touched
) {
    if (x == nullptr || y == nullptr || touched == nullptr) {
        return;
    }

    static int16_t lx = 0, ly = 0;
    static bool pressed = false;

    if (gt_ok) {
        uint8_t st;
        if (gt_read(0x814E, &st, 1) && (st & 0x80)) {      // dato nuevo
            const uint8_t n = st & 0x0F;

            if (n >= 1 && n <= 5) {
                uint8_t p[4];
                if (gt_read(0x8150, p, 4)) {
                    const int px = p[0] | (p[1] << 8);
                    const int py = p[2] | (p[3] << 8);
                    if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
                        lx = px;
                        ly = py;
                        pressed = true;
                    }
                }
            } else {
                pressed = false;                           // 0 toques = soltado
            }

            gt_write8(0x814E, 0);                          // limpiar flag
        }
        // sin dato nuevo: se mantiene el estado anterior
    }

    *x = lx;
    *y = ly;
    *touched = pressed;
}