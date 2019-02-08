#ifndef WINDOW_SENSOR_H
#define WINDOW_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#define LOW 0
#define HIGH 1

struct window_sensor {
    int id;
    int gpio_input;
    int gpio_output;
    char mqtt_topic[128];
    char last_state;
};

struct window_sensor_event {
    struct window_sensor *window_sensor;
    bool level;
};

void window_sensor_init_gpio(void (*isr)(void*), struct window_sensor *window_sensor);

#endif /*WINDOW_SENSOR_H*/
