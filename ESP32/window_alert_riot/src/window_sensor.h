#ifndef WINDOW_SENSOR_H
#define WINDOW_SENSOR_H

#include "periph/gpio.h"

struct window_sensor {
    int id;

    gpio_t gpio_intput;
    gpio_t gpio_output;

    gpio_cb_t isr;
    int interrupt_debounce;
    unsigned long timestamp_last_interrupt;

    char last_state;
    char* mqtt_topic;
};

struct window_sensor_event {
    struct window_sensor* window_sensor;
    bool level;
};

#endif /*WINDOW_SENSOR_H*/
