
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>

// own code
#include "consumption.h"

QueueHandle_t     consumptionQueue;
ConsumptionState  consumptionState;
SemaphoreHandle_t stateLock;

#define TAG "CONSUMPTION"

void lock_state(char *name)
{

    // grab the lock to update the state
    while (xSemaphoreTake(stateLock, portMAX_DELAY) != pdTRUE) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    // ESP_LOGI(TAG, "lock acquired %s", name);
}

void unlock_state(char *name)
{
    // unlock the state
    xSemaphoreGive(stateLock);
    // ESP_LOGI(TAG, "lock released %s", name);
}

////////////////////////////////////////////////////////////////////////////////
// Sun

void set_sun_percentage(int percentage)
{
    // grab the lock to update the state
    lock_state("set sun");

    if (percentage < 0) {
        percentage = 0;
    }
    if (percentage > 100) {
        percentage = 100;
    }
    consumptionState.sunPercentage = percentage;
    unlock_state("set sun");
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
    lock_state("turn off");
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        if (consumptionState.appliances[i] == appliance) {
            consumptionState.appliances[i] = NoAppliance;
        }
    }
    unlock_state("turn off");
}

void turn_on(enum Appliance appliance)
{
    lock_state("turn on");
    // if the device is present, ignore.
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        if (consumptionState.appliances[i] == appliance) {
            goto unlock;
        }
    }
    // add the appliance in the first empty slot
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        if (consumptionState.appliances[i] == NoAppliance) {
            consumptionState.appliances[i] = appliance;
            goto unlock;
        }
    }

unlock:
    unlock_state("turn on");
}

int appliance_consumption(enum Appliance appliance)
{
    switch (appliance) {
    case Car:
        return 1;
    case Dishwasher:
        return 2;
    case Oven:
        return 4;
    default:
        return 0;
    }
}

int total_consumption()
{
    lock_state("total consumption");
    int total = 0;
    int count = (sizeof(consumptionState.appliances) / sizeof(enum Appliance));

    for (int i = 0; i < count; i++) {
        int appliance = appliance_consumption(consumptionState.appliances[i]);

        total = total + appliance;
    }

    unlock_state("total consumption");
    return total;
}

void print_appliance_name(enum Appliance appliace)
{
    switch (appliace) {
    case Car:
        ESP_LOGI(TAG, "Car");
        break;
    case Dishwasher:
        ESP_LOGI(TAG, "Dishwasher");
        break;
    case Oven:
        ESP_LOGI(TAG, "Oven");
        break;
    case Dryer:
        ESP_LOGI(TAG, "Dryer");
        break;
    default:
        ESP_LOGI(TAG, "NoAppliance");
        break;
    }
}

void print_state()
{
    lock_state("print state");
    ESP_LOGI(TAG, "state start -------------------------");
    ESP_LOGI(TAG, "consumption: %d", consumptionState.consumption);
    ESP_LOGI(TAG, "sun: %d", consumptionState.sunPercentage);
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        print_appliance_name(consumptionState.appliances[i]);
    }
    unlock_state("print state");
    ESP_LOGI(TAG, "state end ---------------------------");
}

////////////////////////////////////////////////////////////////////////////////
// generator task

void vConsumptionGenerator(void *params)
{

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int consumption = total_consumption();
        ESP_LOGI(TAG, "current consumption: %d", consumption);
        print_state();
    }

    vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////////////

void init_state()
{
    lock_state("init");
    memset(&consumptionState, 0, sizeof(ConsumptionState));
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        consumptionState.appliances[i] = NoAppliance;
    }
    unlock_state("init");
}

void consumption_set(int consumption)
{
    xQueueSend(consumptionQueue, &consumption, pdMS_TO_TICKS(0));
    return;
}

void initialize_consumption(void)
{
    stateLock = xSemaphoreCreateMutex();
    consumptionQueue = xQueueCreate(1, sizeof(int));

    init_state();

    xTaskCreate(&vConsumptionGenerator, "vConsumptionGenerator", 2048, NULL, 5, NULL);

    return;
}