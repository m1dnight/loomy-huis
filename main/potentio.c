
void vPotentioReaderTask(void *params)
{
    // configure the potentiometer
    // use adc1 and create a handle to the adc.
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT,
    };
}

////////////////////////////////////////////////////////////////////////////////

void initialize(int adc_channel) {}