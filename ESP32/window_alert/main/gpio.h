#ifndef WINDOW_ALERT_GPIO_H
#define WINDOW_ALERT_GPIO_H

void set_gpio_output(int gpio_pin);

void set_gpio_input(int gpio_pin, bool pull_down, bool pull_up, gpio_int_type_t intr_type);

#endif //WINDOW_ALERT_GPIO_H
