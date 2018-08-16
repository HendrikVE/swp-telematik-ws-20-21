#include "MANIFEST.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bmx280_params.h"
#include "bmx280.h"
#include "xtimer.h"

#define CONFIG_DEVICE_ROOM "livingroom"
#define CONFIG_DEVICE_ID "esp32-riot"

#define CONFIG_ESP_WIFI_SSID "SSID"
#define CONFIG_ESP_WIFI_PASSWORD "PASSWORD"

bmx280_t mBme280;

void build_topic(char *output, const char *room, const char *boardID, const char *measurement) {

    sprintf(output, "room/%s/%s/%s", room, boardID, measurement);
}

void publish_environment_data(void) {

    int16_t temperature;
    uint16_t humidity;
    uint32_t pressure;

    temperature = bmx280_read_temperature(&mBme280);
    humidity = bme280_read_humidity(&mBme280);
    pressure = bmx280_read_pressure(&mBme280);

    printf("temperature: ");
    printf("%.1f\n", temperature / 100.0);

    printf("humidity: ");
    printf("%d\n", (int) humidity);

    printf("pressure: ");
    printf("%d\n", (int) pressure);
}

bool setup(void) {

    printf("device is running version: ");
    printf("%s (%i)\n", APP_VERSION_NAME, APP_VERSION_CODE);

    bmx280_params_t mBme280_params = {
        .i2c_dev = I2C_DEV(0),
        .i2c_addr = 0x76,
        .t_sb = BMX280_SB_0_5,
        .filter = BMX280_FILTER_OFF,
        .run_mode = BMX280_MODE_FORCED,
        .temp_oversample = BMX280_OSRS_X16,
        .press_oversample = BMX280_OSRS_X16,
        .humid_oversample = BMX280_OSRS_X16,
    };

    int result = bmx280_init(&mBme280, &mBme280_params);
    if (result == -1) {
        puts("[Error] The given i2c is not enabled");
        return false;
    }

    if (result == -2) {
        printf("[Error] The sensor did not answer correctly at address 0x%02X\n", bmx280_params[0].i2c_addr);
        return false;
    }

    return true;
}

int main(void) {

    bool success = setup();

    if (!success) {
        return false;
    }

    while (1) {
        publish_environment_data();
        xtimer_sleep(2);
    }
}
