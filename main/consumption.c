
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// own code
#include "consumption.h"

QueueHandle_t     consumptionQueue;
ConsumptionState  consumptionState;
SemaphoreHandle_t stateLock;

#define TAG "CONSUMPTION"

void lock_state()
{
    // grab the lock to update the state
    while (xSemaphoreTake(stateLock, portMAX_DELAY) != pdTRUE) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void unlock_state()
{
    // unlock the state
    xSemaphoreGive(stateLock);
}

////////////////////////////////////////////////////////////////////////////////
// Sun

void set_sun_percentage(int percentage)
{
    // grab the lock to update the state
    lock_state();

    if (percentage < 0) {
        percentage = 0;
    }
    if (percentage > 100) {
        percentage = 100;
    }
    consumptionState.sunPercentage = percentage;
    unlock_state();
}

int get_sun_production()
{
    int sun_max_production = 5400;
    int sun_production = sun_max_production * consumptionState.sunPercentage / 100;
    return sun_production;
}

////////////////////////////////////////////////////////////////////////////////
// Appliances

void turn_off(enum Appliance appliance)
{
    lock_state();
    for (int i = 0; i < sizeof(consumptionState.appliances); i++) {
        if (consumptionState.appliances[i] == appliance) {
            consumptionState.appliances[i] = NoAppliance;
            return;
        }
    }
    unlock_state();
}

void turn_on(enum Appliance appliance)
{
    lock_state();
    // if the device is present, ignore.
    for (int i = 0; i < sizeof(consumptionState.appliances); i++) {
        if (consumptionState.appliances[i] == appliance) {
            return;
        }
    }
    // add the appliance in the first empty slot
    for (int i = 0; i < sizeof(consumptionState.appliances); i++) {
        if (consumptionState.appliances[i] == NoAppliance) {
            consumptionState.appliances[i] = appliance;
            return;
        }
    }
    unlock_state();
}

int appliance_consumption(enum Appliance appliance)
{
    switch (appliance) {
    case Car:
        return 9000;
    case Dishwasher:
        return 1500;
    case Oven:
        return 3400;
    default:
        return 0;
    }
}

int total_consumption()
{
    lock_state();
    int total = 0;
    for (int i = 0; i < sizeof(consumptionState.appliances); i++) {
        int appliance = appliance_consumption(consumptionState.appliances[i]);
        total = total + appliance;
    }

    unlock_state();
    return total;
}

////////////////////////////////////////////////////////////////////////////////
// generator task

void vConsumptionGenerator(void *params)
{

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int consumption = total_consumption();
        ESP_LOGI(TAG, "current consumption: %d", consumption);
    }

    vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////////////

void consumption_set(int consumption)
{
    xQueueSend(consumptionQueue, &consumption, pdMS_TO_TICKS(0));
    return;
}

void initialize_consumption(void)
{
    stateLock = xSemaphoreCreateMutex();
    consumptionQueue = xQueueCreate(1, sizeof(int));
    xTaskCreate(&vConsumptionGenerator, "vConsumptionGenerator", 2048, NULL, 5, NULL);

    return;
}