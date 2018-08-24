#ifndef ENVIRONMENT_SENSOR_H
#define ENVIRONMENT_SENSOR_H

#include <stdbool.h>

enum class Sensor {UNDEFINED, BME280, BME680};

class EnvironmentSensor {

public:

    EnvironmentSensor(enum Sensor type);

    bool supportingGasResistance();

    bool begin();

    bool isInitialized();

    float readTemperature();

    float readHumidity();

    float readPressure();

    float readGasResistance();

private:

    Sensor mSensor = Sensor::UNDEFINED;

    Adafruit_BME280* mpBme280;
    Adafruit_BME680* mpBme680;

    Logging logger;

    bool mInitiated = false;

    bool prepareMeasurements();

    static void printTag(Print* _logOutput) {
        char c[12];
        sprintf(c, "%s ", "[EnvironmentSensor] ");
        _logOutput->print(c);
    }

    static void printNewline(Print* _logOutput) {
        _logOutput->print("\n");
    }

};

#endif /*ENVIRONMENT_SENSOR_H*/
