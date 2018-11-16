#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "periph/gpio.h"

#define LOW 0
#define HIGH 1

class WindowSensor {

public:

    WindowSensor(int gpioInput, int gpioOutput, int interruptDebounce, char* mqttTopic) {

        this->msInstanceID++;
        this->mId = msInstanceID;

        this->mGpioInput = gpioInput;
        this->mGpioOutput = gpioOutput;
        this->mInterruptDebounce = interruptDebounce;
        strcpy(this->mMqttTopic, mqttTopic);
    }

    void initGpio(void (*isr)(void*)) {

        gpio_init_int(this->getInputGpio(), GPIO_IN_PD, GPIO_BOTH, isr, NULL);
        gpio_irq_enable(this->getInputGpio());

        gpio_init(this->getOutputGpio(), GPIO_OUT);

        // output always on to detect changes on input
        gpio_write(this->getOutputGpio(), HIGH);
    }

    void initRtcGpio() {

        printf("Not ported yet!\n");

        /*
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
        */
    }

    void deinitRtcGpio() {

        printf("Not ported yet!\n");

        /*
        rtc_gpio_deinit((gpio_num_t) (this->getInputGpio()));
        rtc_gpio_deinit((gpio_num_t) (this->getOutputGpio()));
        */
    }


    /* GETTER */

    int getId() {
        return this->mId;
    }

    int getInputGpio() {
        return this->mGpioInput;
    }

    int getOutputGpio() {
        return this->mGpioOutput;
    }

    unsigned long getTimestampLastInterrupt() {
        return this->mTimestampLastInterrupt;
    }

    int getInterruptDebounce() {
        return this->mInterruptDebounce;
    }

    char getLastState() {
        return this->mLastState;
    }

    char* getMqttTopic() {
        return this->mMqttTopic;
    }


    /* SETTER */

    void setLastState(char state) {
        this->mLastState = state;
    }

    void setTimestampLastInterrupt(int timestamp) {
        this->mTimestampLastInterrupt = timestamp;
    }

private:

    static int msInstanceID;

    int mId;
    int mGpioInput;
    int mGpioOutput;
    int mInterruptDebounce;
    char mMqttTopic[128];
    unsigned long mTimestampLastInterrupt = 0;
    char mLastState;

};

int WindowSensor::msInstanceID = -1;

struct WindowSensorEvent {
    WindowSensor* windowSensor;
    bool level;
};
