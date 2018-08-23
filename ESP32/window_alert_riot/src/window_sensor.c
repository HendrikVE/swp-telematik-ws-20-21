#include "periph/gpio.h"

#include "window_sensor.h"

void init_window_sensor(struct window_sensor ws) {

    gpio_init_int(ws.gpio_intput, GPIO_IN_PD, GPIO_BOTH, ws.isr, NULL);
    gpio_irq_enable(ws.gpio_intput);

    gpio_init(ws.gpio_output, GPIO_OUT);

    // output always on to detect changes on input
    gpio_write(ws.gpio_output, HIGH);
}
