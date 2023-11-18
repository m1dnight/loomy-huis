/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>

// our code
#include "consumption.h"
#include "defines.h"
#include "led_control.h"
#include "potentio.h"
#include "switch.h"

#define TAG "main"

LedControl    *autoLedControl, *kookplaatControl, *droogkastControl, *zonControl;
SwitchControl *autoSwitchControl, *kookplaatSwitchControl, *droogkastSwitchControl;

////////////////////////////////////////////////////////////////////////////////
// LEDs

void initialize_leds()
{
    // auto led
    autoLedControl = create(LED_AUTO_PIN, 12, 1, "AUTO");
    initialize(autoLedControl);
    // post(autoLedControl);

    // kookplaat led
    kookplaatControl = create(LED_KOOKPLAAT_PIN, 8, 0, "KOOKPLAAT");
    initialize(kookplaatControl);
    // post(kookplaatControl);

    // droogkast led
    droogkastControl = create(LED_DROOGKAST_PIN, 7, 1, "KOOKPLAAT");
    initialize(droogkastControl);
    // post(droogkastControl);

    // zon led
    zonControl = create(LED_ZON_PIN, 12, 1, "ZON");
    initialize(zonControl);
    // post(zonControl);
}

void leds_on()
{
    uint8_t value = 128;
    uint8_t red = value;
    uint8_t green = value;
    uint8_t blue = value;
    uint8_t white = value;
    change_color(autoLedControl, red, green, blue, white);
    change_color(kookplaatControl, red, green, blue, white);
    change_color(droogkastControl, red, green, blue, white);
    change_color(zonControl, red, green, blue, white);
    return;
}

////////////////////////////////////////////////////////////////////////////////
// Switches

void auto_switch_toggle(int state)
{
    ESP_LOGI(TAG, "auto switch toggled to %d", state);
    if (state) {
        change_color(autoLedControl, 0, 255, 0, 0);
        turn_on(Car);
    }
    else {
        change_color(autoLedControl, 0, 0, 0, 0);
        turn_off(Car);
    }
    return;
}

void kookplaat_switch_toggle(int state)
{
    ESP_LOGI(TAG, "kookplaat switch toggled to %d", state);
    if (state) {
        change_color(kookplaatControl, 0, 255, 0, 0);
        turn_on(Oven);
    }
    else {
        change_color(kookplaatControl, 0, 0, 0, 0);
        turn_off(Oven);
    }
    return;
}

void droogkast_switch_toggle(int state)
{
    ESP_LOGI(TAG, "droogkast switch toggled to %d", state);
    if (state) {
        change_color(droogkastControl, 0, 255, 0, 0);
        turn_on(Dryer);
    }
    else {
        change_color(droogkastControl, 0, 0, 0, 0);
        turn_off(Dryer);
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
// Potentiometers

void zon_value_change(int value)
{
    // ESP_LOGI(TAG, "zon value changed to %d", value);
    int yellow = (int)(value * 2.5);
    change_color(zonControl, yellow, yellow, 0, yellow);
    set_sun_percentage(value);
    return;
}

void initialize_potentios()
{
    void (*handler)(int);
    handler = &zon_value_change;
    initialize_potentio(ADC_UNIT_1, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11, ADC_CHANNEL_0, handler);
    return;
}

////////////////////////////////////////////////////////////////////////////////

void app_main(void)
{
    initialize_consumption();
    initialize_leds();
    initialize_potentios();
    initialize_switches();

    return;
}
