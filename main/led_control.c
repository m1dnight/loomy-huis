#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// our code
#include "defines.h"
#include "led_control.h"
#include "led_strip_encoder.h"

#define TAG "led_control"

/// @brief updates the pixels with the current buffer
void update_pixels(LedControl *control)
{
    int len = control->ledcount * (3 + control->rgbw);
    ESP_ERROR_CHECK(rmt_transmit(control->led_chan, control->led_encoder, control->led_values, len, &(control->tx_config)));
    return;
}

/// @brief sets the color of single led
void set_color(LedControl *control, int led, uint32_t red, uint32_t green, uint32_t blue, uint32_t white)
{
    if (led >= control->ledcount) {
        ESP_LOGE(TAG, "led %d is out of range; value must be between 0 and %d", led, control->ledcount);
        return;
    }

    int offset = (3 + control->rgbw) * led;

    control->led_values[offset + 0] = green; // groen
    control->led_values[offset + 1] = red;   // rood
    control->led_values[offset + 2] = blue;  // groen?

    if (control->rgbw == 1) {
        control->led_values[offset + 3] = white; // wit
    }
}

/// @brief sets the color of all the leds
void set_all(LedControl *control, uint32_t red, uint32_t green, uint32_t blue, uint32_t white)
{
    for (int i = 0; i < control->ledcount; i++) {
        set_color(control, i, red, green, blue, white);
    }
}

////////////////////////////////////////////////////////////////////////////////

LedControl *create(int pin, int ledcount, int rgbw, char *name)
{
    // alloc for pixel values

    uint8_t *led_values = malloc(sizeof(uint8_t) * ledcount * (3 + rgbw));

    LedControl *ledControl = malloc(sizeof(LedControl));
    ledControl->pin = pin;
    ledControl->rgbw = rgbw;
    ledControl->ledcount = ledcount;
    ledControl->name = name;
    ledControl->led_values = led_values;

    return ledControl;
}

void initialize(LedControl *control)
{

    ESP_LOGI(TAG, "initializing %s", control->name);

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = control->pin,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be
                                // pending in the background
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, (&control->led_chan)));

    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &(control->led_encoder)));
    ESP_ERROR_CHECK(rmt_enable(control->led_chan));

    control->tx_config = (rmt_transmit_config_t){
        .loop_count = 0, // no transfer loop
    };

    // init the pixels on 0
    memset(control->led_values, 0, (3 + control->rgbw) * control->ledcount);
}

void post(LedControl *control)
{

    ESP_LOGI(TAG, "post test %s", control->name);
    // all red
    set_all(control, 255, 0, 0, 0);
    update_pixels(control);
    vTaskDelay(pdMS_TO_TICKS(300));

    // all green
    set_all(control, 0, 255, 0, 0);
    update_pixels(control);
    vTaskDelay(pdMS_TO_TICKS(300));

    // all blue
    set_all(control, 0, 0, 255, 0);
    update_pixels(control);
    vTaskDelay(pdMS_TO_TICKS(300));

    // all white
    set_all(control, 0, 0, 0, 255);
    update_pixels(control);
    vTaskDelay(pdMS_TO_TICKS(300));

    if (control->rgbw == 1) {
        // all white
        set_all(control, 0, 0, 0, 255);
        update_pixels(control);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    else {
        // all white
        set_all(control, 255, 255, 255, 255);
        update_pixels(control);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    // all off
    set_all(control, 0, 0, 0, 0);
    update_pixels(control);

    return;
}

void change_color(LedControl *control, uint32_t red, uint32_t green, uint32_t blue, uint32_t white)
{
    set_all(control, red, green, blue, white);
    update_pixels(control);
    return;
}