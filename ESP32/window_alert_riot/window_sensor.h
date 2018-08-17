#include "periph/gpio.h"

struct window_sensor {
    int id;

    gpio_t gpio_intput;
    gpio_t gpio_output;

    gpio_cb_t isr;
    int interrupt_debounce;

    char* mqtt_topic;
};