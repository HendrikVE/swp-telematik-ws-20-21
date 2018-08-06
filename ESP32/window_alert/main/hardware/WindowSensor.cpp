#include <stdbool.h>

#include "Arduino.h"

class WindowSensor {

private:

    static int instanceID;

    int id;
    int gpio_input;
    int gpio_output;
    int interrupt_debounce;
    char mqtt_topic[128];
    unsigned long timestamp_last_interrupt = 0;
    char last_state;

public:

    WindowSensor(int gpio_input, int gpio_output, int interrupt_debounce, char* mqtt_topic) {

        this->instanceID++;
        this->id = instanceID;

        this->gpio_input = gpio_input;
        this->gpio_output = gpio_output;
        this->interrupt_debounce = interrupt_debounce;
        strcpy(this->mqtt_topic, mqtt_topic);
    }

    void initGPIO(void (*isr)()) {

        pinMode(this->getOutputGPIO(), OUTPUT);
        pinMode(this->getInputGPIO(), INPUT_PULLDOWN);

        attachInterrupt(digitalPinToInterrupt(this->getInputGPIO()), isr, CHANGE);

        // output always on to detect changes on input
        digitalWrite(this->getOutputGPIO(), HIGH);
    }

    void initRTCGPIO() {

        gpio_num_t inputGPIO = (gpio_num_t) (this->getInputGPIO());
        gpio_num_t outputGPIO = (gpio_num_t) (this->getOutputGPIO());

        rtc_gpio_init(inputGPIO);
        rtc_gpio_init(outputGPIO);

        rtc_gpio_set_direction(inputGPIO, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_set_direction(outputGPIO, RTC_GPIO_MODE_OUTPUT_ONLY);

        gpio_pullup_dis(inputGPIO);
        gpio_pulldown_en(inputGPIO);

        rtc_gpio_set_level(outputGPIO, HIGH);
    }

    void deinitRTCGPIO() {

        rtc_gpio_deinit((gpio_num_t) (this->getInputGPIO()));
        rtc_gpio_deinit((gpio_num_t) (this->getOutputGPIO()));
    }


    /* GETTER */

    int getID() {
        return this->id;
    }

    int getInputGPIO() {
        return this->gpio_input;
    }

    int getOutputGPIO() {
        return this->gpio_output;
    }

    int getTimestampLastInterrupt() {
        return this->timestamp_last_interrupt;
    }

    int getInterruptDebounce() {
        return this->interrupt_debounce;
    }

    char getLastState() {
        return this->last_state;
    }

    char* getMQTTTopic() {
        return this->mqtt_topic;
    }


    /* SETTER */

    void setLastState(char state) {
        this->last_state = state;
    }

    void setTimestampLastInterrupt(int timestamp) {
        this->timestamp_last_interrupt = timestamp;
    }

};

int WindowSensor::instanceID = -1;

struct WindowSensorEvent {
    WindowSensor* windowSensor;
    bool level;
};