#ifndef ENVIRONMENT_SENSOR_H
#define ENVIRONMENT_SENSOR_H

#include <stdbool.h>

#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include "Adafruit_BME680.h"
#include "bme680.h"

enum class Sensor {UNDEFINED, BME280, BME680};

class EnvironmentSensor {

public:

    EnvironmentSensor(enum Sensor type);

    bool supportingGasResistance();

    bool begin(int sdaPin, int sclPin);

    bool isInitialized();

    float readTemperature();

    float readHumidity();

    float readPressure();

    float readGasResistance();

private:

    Sensor mSensor = Sensor::UNDEFINED;

    Adafruit_BME280* mpBme280;
    Adafruit_BME680* mpBme680;

    bool mInitiated = false;

    bool prepareMeasurements();
};

#endif /*ENVIRONMENT_SENSOR_H*/
