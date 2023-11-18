#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// own code
#include "defines.h"
#include "potentio.h"

QueueHandle_t adQueue;

#define TAG "ADC"

////////////////////////////////////////////////////////////////////////////////

void vPotentioReaderTask(void *params)
{
    PotentioControl *control = (PotentioControl *)params;

    int measurement;

    while (true) {
        ESP_ERROR_CHECK(adc_oneshot_read(control->adc_handle, control->adc_channel, &measurement));
        xQueueSend(adQueue, &measurement, pdMS_TO_TICKS(0));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void vPotentioHandlerTask(void *params)
{
    PotentioControl *control = (PotentioControl *)params;

    int raw_value = 0;
    while (1) {
        if (xQueueReceive(adQueue, &raw_value, pdMS_TO_TICKS(1000)) == true) {
            double percentage = (double)raw_value / MAX_VOLTAGE * 100;
            int    value = (int)percentage;

            if (value != control->value) {
                control->handler((int)percentage);
                control->value = value;
            }
        }
    }
    vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////////////

PotentioControl *initialize_potentio(int adc_unit, int adc_bandwidth, int adc_attenuation, int adc_channel, void (*handler)(int))
{

    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = adc_unit,
    };

    adc_oneshot_unit_handle_t adc_handle;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = adc_bandwidth,
        .atten = adc_attenuation,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, adc_channel, &config));

    // calibare the adc driver
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = adc_unit,
        .atten = adc_attenuation,
        .bitwidth = adc_bandwidth,
    };

    adc_cali_handle_t handle = NULL;

    int ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "calibration succesful");
    }
    else {
        ESP_LOGE(TAG, "calibration failed");
    }

    PotentioControl *control = malloc(sizeof(PotentioControl));
    control->adc_handle = adc_handle;
    control->handler = handler;
    control->adc_channel = adc_channel;

    // add task to handle changes
    adQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(&vPotentioHandlerTask, "vPotentioHandlerTask", 2048, (void *)control, 5, NULL);
    xTaskCreate(&vPotentioReaderTask, "vPotentioReaderTask", 2048, (void *)control, 5, NULL);
    return control;
}