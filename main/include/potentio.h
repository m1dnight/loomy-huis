#pragma once

typedef struct PotentioControl {
    adc_oneshot_unit_handle_t adc_handle;
    int                       adc_channel;
    void                      (*handler)(int);
    int                       value;
} PotentioControl;

PotentioControl *initialize_potentio(int adc_unit, int adc_bandwidth, int adc_attenuation, int adc_channel, void (*handler)(int));
