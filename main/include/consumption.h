#pragma once

enum Appliance { Car, Dishwasher, Oven, Dryer, NoAppliance };

typedef struct ConsumptionState {
    int            consumption;
    enum Appliance appliances[4];
    int            sunPercentage;
} ConsumptionState;

void initialize_consumption(void);

void consumption_set(int consumption);

void set_sun_percentage(int percentage);

void turn_off(enum Appliance appliance);

void turn_on(enum Appliance appliance);