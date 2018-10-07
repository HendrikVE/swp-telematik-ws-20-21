#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <stdbool.h>

#include "Arduino.h"
#include "ArduinoLog.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"

#define SERVICE_UUID "2fa1dab8-3eef-40fc-8540-7fc496a10d75"

#define CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID              "d3491796-683b-4b9c-aafb-f51a35459d43"
#define CHARACTERISTIC_CONFIG_DEVICE_ID_UUID                "4745e11f-b403-4cfb-83bb-710d46897875"

#define CHARACTERISTIC_CONFIG_OTA_HOST_UUID                 "2f44b103-444c-48f5-bf60-91b81dfa0a25"
#define CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID             "4b95d245-db08-4c56-98f9-738faa8cfbb6"
#define CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID      "1c93dce2-3796-4027-9f55-6d251c41dd34"
#define CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID      "0e837309-5336-45a3-9b69-d0f7134f30ff"

#define CHARACTERISTIC_CONFIG_WIFI_SSID_UUID                "8ca0bf1d-bb5d-4a66-9191-341fd805e288"
#define CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID            "fa41c195-ae99-422e-8f1f-0730702b3fc5"

#define CHARACTERISTIC_CONFIG_MQTT_USER_UUID                "69150609-18f8-4523-a41f-6d9a01d2e08d"
#define CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID            "8bebec77-ea21-4c14-9d64-dbec1fd5267c"
#define CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID           "e3b150fb-90a2-4cd3-80c5-b1189e110ef1"
#define CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID         "4eeff953-0f5e-43ee-b1be-1783a8190b0d"

#define CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID  "68011c92-854a-4f2c-a94c-5ee37dc607c3"

#define STR_IMPL_(x) #x      //stringify argument
#define STR(x) STR_IMPL_(x)  //indirection to expand argument macros

const char* const CHARACTERISTICS[] = {
    CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID,
    CHARACTERISTIC_CONFIG_DEVICE_ID_UUID,
    CHARACTERISTIC_CONFIG_OTA_HOST_UUID,
    CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID,
    CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID,
    CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID,
    CHARACTERISTIC_CONFIG_WIFI_SSID_UUID,
    CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID,
    CHARACTERISTIC_CONFIG_MQTT_USER_UUID,
    CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID,
    CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID,
    CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID,
    CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID
};

const char* const CHARACTERISTIC_VALUES[] = {
        CONFIG_DEVICE_ROOM,
        CONFIG_DEVICE_ID,
        CONFIG_OTA_HOST,
        CONFIG_OTA_FILENAME,
        CONFIG_OTA_SERVER_USERNAME,
        CONFIG_OTA_SERVER_PASSWORD,
        CONFIG_ESP_WIFI_SSID,
        CONFIG_ESP_WIFI_PASSWORD,
        CONFIG_MQTT_USER,
        CONFIG_MQTT_PASSWORD,
        CONFIG_MQTT_SERVER_IP,
        STR(CONFIG_MQTT_SERVER_PORT),
        STR(CONFIG_SENSOR_POLL_INTERVAL_MS)
};

class ConnectivityManager {

public:

    ConnectivityManager();

    void begin();

    bool checkWifiConnection();
    bool initWifi(const char* ssid, const char* password);
    void turnOnWifi();
    void turnOffWifi();

    bool checkMqttConnection();
    bool initMqtt(const char* address, int port, const char* user, const char* password, const char* clientID);
    MQTTClient* getMqttClient();

    bool initBluetoothConfig(BLECharacteristicCallbacks* callbacks);
    void turnOnBluetooth();
    void turnOffBluetooth();

private:

    WiFiClientSecure mWifiClientSecure;
    MQTTClient mMqttClient;

    const char* mMqttUser;
    const char* mMqttPassword;
    const char* mMqttClientID;

    Logging logger;

    SemaphoreHandle_t mWifiMutex = NULL;
    SemaphoreHandle_t mMqttMutex = NULL;

    void static wifiEvent(WiFiEvent_t event) {

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

    static void printTag(Print* _logOutput) {
        char c[12];
        sprintf(c, "%s ", "[ConnectivityManager] ");
        _logOutput->print(c);
    }

    static void printNewline(Print* _logOutput) {
        _logOutput->print("\n");
    }

};

#endif /*CONNECTIVITY_MANAGER_H*/
