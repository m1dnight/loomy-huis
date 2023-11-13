#pragma once

typedef struct SwitchControl {
    void (*handler)(int);
    int  pin;
    int  state;
} SwitchControl;

SwitchControl *create_switch(int pin, void (*handler)(int));

void initialize_switch(SwitchControl *control);
