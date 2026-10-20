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
        setPanel(&_panel_instance);
    }
};

static LGFX lcd;

// ---------------------------------------------------------------------------
// PCA9557 (expansor de la CrowPanel V3.0)
// Registros: 0=entrada, 1=salida, 2=polaridad, 3=configuracion (1=entrada)
// ---------------------------------------------------------------------------
static constexpr uint8_t PCA9557_ADDR = 0x18;

static bool pca_write(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(PCA9557_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

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

static void i2c_scan() {
    Serial.println("[I2C] Escaneando bus...");
    int found = 0;
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C]  dispositivo en 0x%02X\n", a);
            found++;
        }
    }
    Serial.printf("[I2C] %d dispositivo(s)\n", found);
}

// Reset del GT911 usando el PCA9557.
// rst_mask: bit del PCA conectado al RST del GT911 (0x01 o 0x02)
// int_mask: el otro bit (INT). INT bajo durante reset -> 0x5D, alto -> 0x14
static void touch_reset(uint8_t rst_mask, bool int_high) {
    const uint8_t int_mask = (rst_mask == 0x01) ? 0x02 : 0x01;

    pca_write(0x03, 0x00);                                  // todos salida
    pca_write(0x01, int_high ? int_mask : 0x00);            // RST bajo, INT segun direccion
    delay(20);
    pca_write(0x01, (int_high ? int_mask : 0x00) | rst_mask); // liberar RST
    delay(10);
    pca_write(0x03, int_mask);                              // INT pasa a entrada
    delay(150);                                             // GT911 necesita tiempo
}

static bool gt_probe() {
    for (uint8_t a : {(uint8_t)0x5D, (uint8_t)0x14}) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            gt_addr = a;
            return true;
        }
    }
    return false;
}

static bool gt_init() {
    const uint8_t rst_opts[] = {0x01, 0x02};

    for (uint8_t rst : rst_opts) {
        for (int int_high = 0; int_high <= 1; int_high++) {
            Serial.printf("[TOUCH] Intento: RST=IO%d, INT %s\n",
                          (rst == 0x01) ? 0 : 1, int_high ? "alto" : "bajo");
            touch_reset(rst, int_high);

            if (gt_probe()) {
                uint8_t id[4] = {0};
                gt_read(0x8140, id, 4);
                Serial.printf("[TOUCH] GT911 en 0x%02X, ID=%c%c%c%c\n",
                              gt_addr,
                              id[0] ? id[0] : '?',
                              id[1] ? id[1] : '?',
                              id[2] ? id[2] : '?',
                              id[3] ? id[3] : '?');

                // Volcado de la configuracion del chip
                uint8_t c[6] = {0};
                if (gt_read(0x8047, c, 6)) {
                    Serial.printf("[TOUCH] cfg ver=0x%02X  Xmax=%d  Ymax=%d  maxToques=%d\n",
                                  c[0],
                                  c[1] | (c[2] << 8),
                                  c[3] | (c[4] << 8),
                                  c[5] & 0x0F);
                                  uint8_t m[1];
                    if (gt_read(0x804D, m, 1)) Serial.printf("[TOUCH] ModuleSwitch1=0x%02X\n", m[0]);
                    if (gt_read(0x804E, m, 1)) Serial.printf("[TOUCH] ModuleSwitch2=0x%02X\n", m[0]);
                    if (gt_read(0x8056, m, 1)) Serial.printf("[TOUCH] RefreshRate=%d\n", m[0]);

                } else {
                    Serial.println("[TOUCH] No pude leer la configuracion");
                }

                gt_write8(0x814E, 0);   // limpiar flag inicial
                return true;
            }
        }
    }

    Serial.println("[TOUCH] GT911 NO responde con ninguna combinacion");
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
    Wire.setClock(100000);       // 100 kHz durante la deteccion
    Wire.setTimeOut(50);

    i2c_scan();

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);       // backlight

    lcd.init();
    lcd.setBrightness(255);
    Serial.println("[LCD] LovyanGFX inicializado");

    Serial.println("[TOUCH] Inicializando GT911...");
    gt_ok = gt_init();
    if (gt_ok) {
        Wire.setClock(400000);
    } else {
        i2c_scan();              // para ver que hay en el bus tras los resets
    }
    Serial.println("[TOUCH] Inicializacion terminada");

    lv_init();

    // Buffers LVGL en RAM interna
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

    static int16_t  lx = 0, ly = 0;
    static bool     pressed = false;
    static uint8_t  last_st = 0xFF;
    static uint32_t last_hb = 0;
    static uint32_t read_fail = 0, read_ok = 0;

    if (gt_ok) {
        // Estado + primer punto en una sola lectura: 0x814E..0x8155
        uint8_t b[8] = {0};

        if (gt_read(0x814E, b, 8)) {
            read_ok++;
            const uint8_t st = b[0];

            // Log SIEMPRE que cambie el registro de estado
            if (st != last_st) {
                last_st = st;
                Serial.printf("[TOUCH] st=0x%02X  datos: %02X %02X %02X %02X %02X %02X %02X\n",
                              st, b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
            }

            if (st & 0x80) {                               // dato nuevo
                const uint8_t n = st & 0x0F;

                if (n >= 1 && n <= 5) {
                    // b[1]=id, b[2..3]=X, b[4..5]=Y
                    const int px = b[2] | (b[3] << 8);
                    const int py = b[4] | (b[5] << 8);

                    Serial.printf("[TOUCH] n=%d X=%d Y=%d %s\n",
                                  n, px, py,
                                  (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) ? "" : "(FUERA DE RANGO)");

                    if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
                        lx = px;
                        ly = py;
                        pressed = true;
                    }
                } else {
                    pressed = false;                       // 0 toques = soltado
                }

                gt_write8(0x814E, 0);                      // limpiar flag
            }
        } else {
            read_fail++;
        }
    }

    if (millis() - last_hb > 3000) {
        last_hb = millis();
        Serial.printf("[TOUCH] hb ok=%lu fail=%lu pressed=%d last_st=0x%02X\n",
                      (unsigned long)read_ok,
                      (unsigned long)read_fail,
                      pressed ? 1 : 0,
                      last_st);
    }

    *x = lx;
    *y = ly;
    *touched = pressed;
}