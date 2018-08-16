#include "MANIFEST.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "saul_reg.h"
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

    phydat_t temperature;
    phydat_t humidity;
    phydat_t pressure;

    saul_reg_t* devTemp = saul_reg_find_type(SAUL_SENSE_TEMP);
    saul_reg_t* devHum = saul_reg_find_type(SAUL_SENSE_HUM);
    saul_reg_t* devPres = saul_reg_find_type(SAUL_SENSE_PRESS);

    saul_reg_read(devTemp, &temperature);
    saul_reg_read(devHum, &humidity);
    saul_reg_read(devPres, &pressure);


    printf("temperature: ");
    printf("%.1f\n", round(temperature.val[0] * pow(10, temperature.scale) * 10.0) / 10.0);

    printf("humidity: ");
    printf("%d\n", (int) round(humidity.val[0] * pow(10, humidity.scale)));

    printf("pressure: ");
    printf("%d\n", (int) round(pressure.val[0] * pow(10, pressure.scale)));
}

bool setup(void) {

    printf("device is running version: ");
    printf("%s (%i)\n", APP_VERSION_NAME, APP_VERSION_CODE);

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
