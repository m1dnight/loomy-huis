
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

// own code
#include "consumption.h"
#include "mqtt.h"

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

int index_of_appliance(enum Appliance appliance)
{
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        if (consumptionState.appliances[i] == appliance) {
            return i;
        }
    }
    return -1;
}

void add_appliance(enum Appliance appliance)
{
    // add the appliance in the first empty slot
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        if (consumptionState.appliances[i] == NoAppliance) {
            consumptionState.appliances[i] = appliance;
            return;
        }
    }
}

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
    int appliance_index = index_of_appliance(appliance);

    if (appliance_index < 0) {
        add_appliance(appliance);
    }

    unlock_state("turn on");
}

int appliance_consumption(enum Appliance appliance)
{
    switch (appliance) {
    case Car:
        return 11000;
    case Dishwasher:
        return 1400;
    case Oven:
        return 3500;
    case Dryer:
        return 3500;
    default:
        return 0;
    }
}

int total_consumption()
{
    lock_state("total consumption");
    // compute the total consumption
    int total_consumption = 0;
    for (int i = 0; i < (sizeof(consumptionState.appliances) / sizeof(enum Appliance)); i++) {
        int appliance = appliance_consumption(consumptionState.appliances[i]);
        total_consumption += appliance;
    }

    // compute the total production
    int total_production = get_sun_production();

    unlock_state("total consumption");
    return total_consumption - total_production;
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

double random_factor(double min, double max)
{
    uint32_t random = esp_random();
    return (double)random / (double)(UINT32_MAX)*max + min;
}

void generate_message(char *message, int total_consumption)
{
    // el:13752.825000so:14184.295000in:0.000000co:0.397000ts:231119154214Wgs:2582.466000mpt:231114123000Wmp:3.668000ps:
    // build the timestamp
    char   timestring[64];
    time_t now = 0;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    strftime(timestring, sizeof(timestring), "%y%m%d%H%M%SW", timeinfo);

    // compute actual consumption values
    double consumption = 0.0;
    double injection = 0.0;

    if (total_consumption > 0) {
        consumption = total_consumption * 0.001+ random_factor(0.0, 0.01);
    }
    else {
        injection = total_consumption * -0.001 + random_factor(0.0, 0.01);
    }

    // build the message

    sprintf(message, "el:%sso:%sin:%fco:%fts:%sgs:%smpt:%smp:%sps:", "13752.825000", "14184.295000", injection, consumption, timestring, "2582.466000",
            "231114123000W", "3.668000");

    ESP_LOGI(TAG, "message: %s", message);
}

void vConsumptionGenerator(void *params)
{

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(500));
        int consumption = total_consumption();
        ESP_LOGI(TAG, "current consumption: %d", consumption);

        char message[128];

        generate_message(message, consumption);
        publish(message);
        // print_state();
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

    xTaskCreate(&vConsumptionGenerator, "vConsumptionGenerator", 4096, NULL, 5, NULL);

    return;
}