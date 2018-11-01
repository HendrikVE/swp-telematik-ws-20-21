#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <stdbool.h>

#include "Arduino.h"
#include "ArduinoLog.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"

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

    bool initBluetoothConfig(const char* serviceUuid, BLECharacteristicCallbacks* callbacks, std::map <std::string, std::string>& characteristics);
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
