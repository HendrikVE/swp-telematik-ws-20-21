#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"

#include "Arduino.h"
#include "WiFi.h"

#include "driver/gpio.h"
#include "esp_event_loop.h"

#define LOW 0
#define HIGH 1

#define ESP_INTR_FLAG_DEFAULT 0

extern const uint8_t iot_eclipse_org_pem_start[] asm("_binary_iot_eclipse_org_pem_start");
extern const uint8_t iot_eclipse_org_pem_end[] asm("_binary_iot_eclipse_org_pem_end");

BME280I2C bme;

void WiFiEvent(WiFiEvent_t event) {

    switch(event) {

        case SYSTEM_EVENT_STA_START:
            Serial.println("SYSTEM_EVENT_STA_START");
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("SYSTEM_EVENT_STA_GOT_IP");
            break;

        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("SYSTEM_EVENT_AP_STACONNECTED");
            break;

        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("SYSTEM_EVENT_AP_STADISCONNECTED");
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("SYSTEM_EVENT_STA_DISCONNECTED");
            break;

        default:
            break;
    }
}

void checkWiFiConnection() {

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Trying to connect to WiFi...");
        WiFi.begin(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
        delay(10000);
    }
}

void initWiFi() {

    WiFi.onEvent(WiFiEvent);
    checkWiFiConnection();
}
void setup(){

    Serial.begin(115200);

    initWiFi();
}

void loop(){
    Serial.println("loop");
    checkWiFiConnection();

    delay(1000);
}
