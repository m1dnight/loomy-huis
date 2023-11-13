#pragma once

#include "driver/rmt_tx.h"
#include <stdio.h>

typedef struct LedControl {
    char                 *name;
    int                   rgbw;
    uint8_t              *led_values;
    int                   ledcount;
    int                   pin;
    rmt_channel_handle_t  led_chan;
    rmt_transmit_config_t tx_config;
    rmt_encoder_handle_t  led_encoder;
} LedControl;

LedControl *create(int pin, int ledcount, int rgbw, char *name);

void initialize(LedControl *ledControl);

void post(LedControl *ledControl);

void change_color(LedControl *control, uint32_t red, uint32_t green, uint32_t blue, uint32_t white);