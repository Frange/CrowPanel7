
#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#include <Arduino.h>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480

void display_init();

void display_get_touch(
    int16_t *x,
    int16_t *y,
    bool *touched
);

#endif
