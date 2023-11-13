#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// own code
#include "defines.h"
#include "switch.h"

QueueHandle_t interruptQueue;

#define TAG "switch"

static void IRAM_ATTR gpio_isr_handler(void *args)
{
    SwitchControl *control = (SwitchControl *)args;
    xQueueSendFromISR(interruptQueue, &control, NULL);
}

void switchFlippedTask(void *params)
{
    SwitchControl *control = 0;

    while (true) {
        if (xQueueReceive(interruptQueue, &control, portMAX_DELAY)) {
            // disable the interrupt for a bit
            gpio_isr_handler_remove(control->pin);

            // delay the readout a bit to ensure the button is stable
            vTaskDelay(pdMS_TO_TICKS(10));

            bool on = gpio_get_level(control->pin);
            bool changed = control->state != on;

            if (changed) {
                control->handler(on);
                control->state = on;
            }

            // enable the interrupt again
            gpio_isr_handler_add(control->pin, gpio_isr_handler, (void *)control);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

SwitchControl *create_switch(int pin, void (*handler)(int))
{
    SwitchControl *switchControl = malloc(sizeof(SwitchControl));
    switchControl->pin = pin;
    switchControl->handler = handler;
    switchControl->state = -1; // set the switch to an invalid state at config
    return switchControl;
}

void initialize_switch(SwitchControl *control)
{
    gpio_set_direction(control->pin, GPIO_MODE_INPUT);
    gpio_pulldown_en(control->pin);
    gpio_pullup_dis(control->pin);
    gpio_set_intr_type(control->pin, GPIO_INTR_POSEDGE);

    // // queue to handle events of switch changes
    interruptQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(switchFlippedTask, "switchFlippedTask", 2048, NULL, 1, NULL);

    // install the interrupt
    gpio_install_isr_service(0);
    gpio_isr_handler_add(control->pin, gpio_isr_handler, (void *)control);
}
