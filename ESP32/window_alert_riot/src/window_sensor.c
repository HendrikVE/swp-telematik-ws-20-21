#include "window_sensor.h"

#include "stdlib.h"

#include "periph/gpio.h"

void window_sensor_init_gpio(void (*isr)(void*), struct window_sensor *window_sensor) {

    gpio_init_int(window_sensor->gpio_input, GPIO_IN_PD, GPIO_BOTH, isr, NULL);
    gpio_irq_enable(window_sensor->gpio_input);

    gpio_init(window_sensor->gpio_output, GPIO_OUT);

    // output always on to detect changes on input
    gpio_write(window_sensor->gpio_output, HIGH);
}
