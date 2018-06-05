#include "driver/gpio.h"

#include "gpio.h"

void set_gpio_output(int gpio_pin) {

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = (1ULL << gpio_pin);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

void set_gpio_input(int gpio_pin, bool pull_down, bool pull_up, gpio_int_type_t intr_type) {

    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = intr_type;
    //bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << gpio_pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = pull_down;
    io_conf.pull_up_en = pull_up;
    gpio_config(&io_conf);
}
