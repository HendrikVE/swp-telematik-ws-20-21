#include <stdbool.h>

#include "ArduinoLog.h"
#include "Wire.h"

#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include "Adafruit_BME680.h"

#define BME_280_I2C_ADDRESS 0x76
#define BME_680_I2C_ADDRESS 0x77

enum class Sensor {UNDEFINED, BME280, BME680};

class EnvironmentSensor {

public:

    EnvironmentSensor(enum Sensor type) {
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

    bool supportingGasResistance() {
        return mSensor == Sensor::BME680;
    }

    bool begin() {

        logger.begin(LOG_LEVEL_VERBOSE, &Serial);
        logger.setPrefix(printTag);
        logger.setSuffix(printNewline);

        if (mSensor == Sensor::BME280) {
            Wire.begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SDC_GPIO_PIN);

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
            Wire.begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SDC_GPIO_PIN);

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

    bool isInitialized() {
        return this->mInitiated;
    }

    float readTemperature() {

        prepareMeasurements();

        if (mSensor == Sensor::BME280) {
            return mpBme280->readTemperature();
        }
        else if (mSensor == Sensor::BME680) {
            return mpBme680->temperature;
        }

        throw "Sensor type is UNDEFINED!";
    }

    float readHumidity() {

        prepareMeasurements();

        if (mSensor == Sensor::BME280) {
            return mpBme280->readHumidity();
        }
        else if (mSensor == Sensor::BME680) {
            return mpBme680->humidity;
        }

        throw "Sensor type is UNDEFINED!";
    }

    float readPressure() {

        prepareMeasurements();

        if (mSensor == Sensor::BME280) {
            return mpBme280->readPressure();
        }
        else if (mSensor == Sensor::BME680) {
            return mpBme680->pressure;
        }

        throw "Sensor type is UNDEFINED!";
    }

    float readGasResistance() {

        prepareMeasurements();

        if (mSensor == Sensor::BME280) {
            throw "BME280 does not support gas measurements!";
        }
        else if (mSensor == Sensor::BME680) {
            return mpBme680->gas_resistance;
        }

        throw "Sensor type is UNDEFINED!";
    }

private:

    Sensor mSensor = Sensor::UNDEFINED;

    Adafruit_BME280* mpBme280;
    Adafruit_BME680* mpBme680;

    Logging logger;

    bool mInitiated = false;

    bool prepareMeasurements() {

        if (mSensor == Sensor::BME680) {
            if (!mpBme680->performReading()) {
                logger.notice("Failed to perform reading");
                return false;
            }

            return true;
        }

        return true;
    }

    static void printTag(Print* _logOutput) {
        char c[12];
        sprintf(c, "%s ", "[EnvironmentSensor] ");
        _logOutput->print(c);
    }

    static void printNewline(Print* _logOutput) {
        _logOutput->print("\n");
    }

};
