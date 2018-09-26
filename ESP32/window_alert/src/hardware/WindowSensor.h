#ifndef WINDOW_SENSOR_H
#define WINDOW_SENSOR_H

#include <stdbool.h>
#include <string.h>

#define HIGH 1
#define LOW 0

class WindowSensor {

public:

    WindowSensor(int gpioInput, int gpioOutput, int interruptDebounce, char* mqttTopic);

    void initGpio(void (*isr)(void*));

    void initRtcGpio();

    void deinitRtcGpio();


    /* GETTER */

    int getId();

    int getInputGpio();

    int getOutputGpio();

    unsigned long getTimestampLastInterrupt();

    int getInterruptDebounce();

    char getLastState();

    char* getMqttTopic();


    /* SETTER */

    void setLastState(char state);

    void setTimestampLastInterrupt(int timestamp);

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

struct WindowSensorEvent {
    WindowSensor* windowSensor;
    bool level;
};

#endif /*WINDOW_SENSOR_H*/
