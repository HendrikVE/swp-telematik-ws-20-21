#include <stdbool.h>

#include "Wire.h"

#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include "Adafruit_BME680.h"

#define BME_280_I2C_ADDRESS 0x76
#define BME_680_I2C_ADDRESS 0x77

enum class Sensor {UNDEFINED, BME280, BME680};

class EnvironmentSensor {

private:

    Sensor sensor = Sensor::UNDEFINED;

    Adafruit_BME280* bme280;
    Adafruit_BME680* bme680;

    bool prepareMeasurements() {

        if (sensor == Sensor::BME680) {
            if (!bme680->performReading()) {
                Serial.println("Failed to perform reading");
                return false;
            }

            return true;
        }

        return true;
    }

public:

    EnvironmentSensor(enum Sensor type) {
        sensor = type;

        if (type == Sensor::BME280) {
            bme280 = new Adafruit_BME280();
        }
        else if (type == Sensor::BME680) {
            bme680 = new Adafruit_BME680();
        }
        else if (type == Sensor::UNDEFINED) {
            throw "Undefined sensor type not allowed!";
        }
    }

    bool supportingGasResistence() {
        return sensor == Sensor::BME680;
    }

    void init() {

        if (sensor == Sensor::BME280) {
            Wire.begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SDC_GPIO_PIN);

            while(!bme280->begin(BME_280_I2C_ADDRESS)) {
                Serial.println("Could not find BME280 sensor!");
                delay(1000);
            }
        }
        else if (sensor == Sensor::BME680) {
            Wire.begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SDC_GPIO_PIN);

            while(!bme680->begin(BME_680_I2C_ADDRESS)) {
                Serial.println("Could not find BME680 sensor!");
                delay(1000);
            }
        }
    }

    float readTemperature() {

        prepareMeasurements();

        if (sensor == Sensor::BME280) {
            return bme280->readTemperature();
        }
        else if (sensor == Sensor::BME680) {
            return bme680->temperature;
        }

        throw "Sensor type is UNDEFINED!";
    }

    float readHumidity() {

        prepareMeasurements();

        if (sensor == Sensor::BME280) {
            return bme280->readHumidity();
        }
        else if (sensor == Sensor::BME680) {
            return bme680->humidity;
        }

        throw "Sensor type is UNDEFINED!";
    }

    float readPressure() {

        prepareMeasurements();

        if (sensor == Sensor::BME280) {
            return bme280->readPressure();
        }
        else if (sensor == Sensor::BME680) {
            return bme680->pressure;
        }

        throw "Sensor type is UNDEFINED!";
    }

    float readGasResistence() {

        prepareMeasurements();

        if (sensor == Sensor::BME280) {
            throw "BME280 does not support gas measurements!";
        }
        else if (sensor == Sensor::BME680) {
            return bme680->gas_resistance;
        }

        throw "Sensor type is UNDEFINED!";
    }

};