#include <stdbool.h>

#include "ArduinoLog.h"
#include "Wire.h"

#include "EnvironmentSensor.h"

#define BME_280_I2C_ADDRESS 0x76
#define BME_680_I2C_ADDRESS 0x77

EnvironmentSensor::EnvironmentSensor(enum Sensor type) {
    mSensor = type;

    if (type == Sensor::BME280) {
        mpBme280 = new Adafruit_BME280();
    }
    else if (type == Sensor::BME680) {
        mpBme680 = new Adafruit_BME680();
    }
    else if (type == Sensor::UNDEFINED) {
        throw "Undefined sensor type not allowed!";
    }
}

bool EnvironmentSensor::supportingGasResistance() {
    return mSensor == Sensor::BME680;
}

bool EnvironmentSensor::begin(int sdaPin, int sclPin) {

    logger.begin(LOG_LEVEL_VERBOSE, &Serial);
    logger.setPrefix(printTag);
    logger.setSuffix(printNewline);

    Wire.begin(sdaPin, sclPin);

    if (mSensor == Sensor::BME280) {

        int attempts = 0;
        while(!mpBme280->begin(BME_280_I2C_ADDRESS)) {
            logger.notice("Could not find BME280 sensor!");

            attempts++;
            if (attempts >= 10) {
                return false;
            }

            delay(500);
        }
    }
    else if (mSensor == Sensor::BME680) {

        int attempts = 0;
        while(!mpBme680->begin(BME_680_I2C_ADDRESS)) {
            logger.notice("Could not find BME680 sensor!");

            attempts++;
            if (attempts >= 10) {
                return false;
            }

            delay(500);
        }
    }

    this->mInitiated = true;

    return true;
}

bool EnvironmentSensor::isInitialized() {
    return this->mInitiated;
}

float EnvironmentSensor::readTemperature() {

    prepareMeasurements();

    if (mSensor == Sensor::BME280) {
        return mpBme280->readTemperature();
    }
    else if (mSensor == Sensor::BME680) {
        return mpBme680->temperature;
    }

    throw "Sensor type is UNDEFINED!";
}

float EnvironmentSensor::readHumidity() {

    prepareMeasurements();

    if (mSensor == Sensor::BME280) {
        return mpBme280->readHumidity();
    }
    else if (mSensor == Sensor::BME680) {
        return mpBme680->humidity;
    }

    throw "Sensor type is UNDEFINED!";
}

float EnvironmentSensor::readPressure() {

    prepareMeasurements();

    if (mSensor == Sensor::BME280) {
        return mpBme280->readPressure();
    }
    else if (mSensor == Sensor::BME680) {
        return mpBme680->pressure;
    }

    throw "Sensor type is UNDEFINED!";
}

float EnvironmentSensor::readGasResistance() {

    prepareMeasurements();

    if (mSensor == Sensor::BME280) {
        throw "BME280 does not support gas measurements!";
    }
    else if (mSensor == Sensor::BME680) {
        return mpBme680->gas_resistance;
    }

    throw "Sensor type is UNDEFINED!";
}

bool EnvironmentSensor::prepareMeasurements() {

    if (mSensor == Sensor::BME680) {
        if (!mpBme680->performReading()) {
            logger.notice("Failed to perform reading");
            return false;
        }

        return true;
    }

    return true;
}
