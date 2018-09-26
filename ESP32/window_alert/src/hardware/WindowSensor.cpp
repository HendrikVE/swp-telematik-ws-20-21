#include <stdbool.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "WindowSensor.h"

#define ESP_INTR_FLAG_DEFAULT 0

int WindowSensor::msInstanceID = -1;

WindowSensor::WindowSensor(int gpioInput, int gpioOutput, int interruptDebounce, char* mqttTopic) {

    this->msInstanceID++;
    this->mId = msInstanceID;

    this->mGpioInput = gpioInput;
    this->mGpioOutput = gpioOutput;
    this->mInterruptDebounce = interruptDebounce;
    strcpy(this->mMqttTopic, mqttTopic);
}

void set_gpio_output(int gpio_pin) {

    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;

    io_conf.pin_bit_mask = (1ULL << gpio_pin);

    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    gpio_config(&io_conf);
}

void set_gpio_input(int gpio_pin, bool pull_down, bool pull_up, gpio_int_type_t intr_type) {

    gpio_config_t io_conf;

    io_conf.intr_type = intr_type;
    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pin_bit_mask = (1ULL << gpio_pin);
    
    io_conf.pull_down_en = pull_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;

    gpio_config(&io_conf);
}

void WindowSensor::initGpio(void (*isr)(void*)) {

    set_gpio_output(this->getOutputGpio());
    set_gpio_input(this->getInputGpio(), true, false, GPIO_INTR_ANYEDGE);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add((gpio_num_t) this->getInputGpio(), isr, (void*) this->getInputGpio());

    // output always on to detect changes on input
    gpio_set_level((gpio_num_t) this->getOutputGpio(), HIGH);
}

void WindowSensor::initRtcGpio() {

    gpio_num_t inputGPIO = (gpio_num_t) (this->getInputGpio());
    gpio_num_t outputGPIO = (gpio_num_t) (this->getOutputGpio());

    rtc_gpio_init(inputGPIO);
    rtc_gpio_init(outputGPIO);

    rtc_gpio_set_direction(inputGPIO, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_set_direction(outputGPIO, RTC_GPIO_MODE_OUTPUT_ONLY);

    gpio_pullup_dis(inputGPIO);
    gpio_pulldown_en(inputGPIO);

    rtc_gpio_set_level(outputGPIO, HIGH);

    rtc_gpio_hold_en(inputGPIO);
    rtc_gpio_hold_en(outputGPIO);
}

void WindowSensor::deinitRtcGpio() {

    rtc_gpio_deinit((gpio_num_t) (this->getInputGpio()));
    rtc_gpio_deinit((gpio_num_t) (this->getOutputGpio()));
}


/* GETTER */

int WindowSensor::getId() {
    return this->mId;
}

int WindowSensor::getInputGpio() {
    return this->mGpioInput;
}

int WindowSensor::getOutputGpio() {
    return this->mGpioOutput;
}

unsigned long WindowSensor::getTimestampLastInterrupt() {
    return this->mTimestampLastInterrupt;
}

int WindowSensor::getInterruptDebounce() {
    return this->mInterruptDebounce;
}

char WindowSensor::getLastState() {
    return this->mLastState;
}

char* WindowSensor::getMqttTopic() {
    return this->mMqttTopic;
}


/* SETTER */

void WindowSensor::setLastState(char state) {
    this->mLastState = state;
}

void WindowSensor::setTimestampLastInterrupt(int timestamp) {
    this->mTimestampLastInterrupt = timestamp;
}
