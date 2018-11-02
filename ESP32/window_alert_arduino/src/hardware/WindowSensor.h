#ifndef WINDOW_SENSOR_H
#define WINDOW_SENSOR_H

#include <stdbool.h>
#include <string>

#include "Arduino.h"

class WindowSensor {

public:

    WindowSensor(int gpioInput, int gpioOutput, int interruptDebounce, std::string mqttTopic);

    void initGpio(void (*isr)());

    void initRtcGpio();

    void deinitRtcGpio();


    /* GETTER */

    int getId();

    int getInputGpio();

    int getOutputGpio();

    unsigned long getTimestampLastInterrupt();

    int getInterruptDebounce();

    char getLastState();

    std::string getMqttTopic();


    /* SETTER */

    void setLastState(char state);

    void setTimestampLastInterrupt(int timestamp);

private:

    static int msInstanceID;

    int mId;
    int mGpioInput;
    int mGpioOutput;
    int mInterruptDebounce;
    std::string mMqttTopic;
    unsigned long mTimestampLastInterrupt = 0;
    char mLastState;

};

struct WindowSensorEvent {
    WindowSensor* windowSensor;
    bool level;
};

#endif /*WINDOW_SENSOR_H*/
