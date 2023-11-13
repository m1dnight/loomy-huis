/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>

// our code
#include "defines.h"
#include "led_control.h"
#include "switch.h"

#define TAG "main"

LedControl    *autoLedControl, *kookplaatControl, *droogkastControl;
SwitchControl *autoSwitchControl, *kookplaatSwitchControl, *droogkastSwitchControl;

////////////////////////////////////////////////////////////////////////////////
// LEDs

void initialize_leds()
{
    // auto led
    autoLedControl = create(AUTO_PIN, 12, 1, "AUTO");
    initialize(autoLedControl);
    post(autoLedControl);

    // kookplaat led
    kookplaatControl = create(KOOKPLAAT_PIN, 8, 0, "KOOKPLAAT");
    initialize(kookplaatControl);
    post(kookplaatControl);

    // droogkast led
    droogkastControl = create(DROOGKAST_PIN, 7, 1, "KOOKPLAAT");
    initialize(droogkastControl);
    post(droogkastControl);
}

////////////////////////////////////////////////////////////////////////////////
// Switches

void auto_switch_toggle(int state)
{
    ESP_LOGI(TAG, "auto switch toggled to %d", state);
    if (state) {
        change_color(autoLedControl, 0, 255, 0, 0);
    }
    else {
        change_color(autoLedControl, 0, 0, 0, 0);
    }
    return;
}

void kookplaat_switch_toggle(int state)
{
    ESP_LOGI(TAG, "kookplaat switch toggled to %d", state);
    if (state) {
        change_color(kookplaatControl, 0, 255, 0, 0);
    }
    else {
        change_color(kookplaatControl, 0, 0, 0, 0);
    }
    return;
}

void droogkast_switch_toggle(int state)
{
    ESP_LOGI(TAG, "droogkast switch toggled to %d", state);
    if (state) {
        change_color(droogkastControl, 0, 255, 0, 0);
    }
    else {
        change_color(droogkastControl, 0, 0, 0, 0);
    }
    return;
}

void initialize_switches()
{
    void (*handler)(int);

    handler = &auto_switch_toggle;
    autoSwitchControl = create_switch(AUTO_SWITCH_PIN, handler);
    initialize_switch(autoSwitchControl);

    handler = &kookplaat_switch_toggle;
    kookplaatSwitchControl = create_switch(KOOKPLAAT_SWITCH_PIN, handler);
    initialize_switch(kookplaatSwitchControl);

    handler = &droogkast_switch_toggle;
    droogkastSwitchControl = create_switch(DROOGKAST_SWITCH_PIN, handler);
    initialize_switch(droogkastSwitchControl);

    return;
}

////////////////////////////////////////////////////////////////////////////////

void app_main(void)
{
    initialize_leds();
    initialize_switches();
    return;
}
