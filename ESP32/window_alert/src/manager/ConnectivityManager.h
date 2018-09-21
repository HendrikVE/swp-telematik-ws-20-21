#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <stdbool.h>

#include "Arduino.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"

class ConnectivityManager {

public:

    ConnectivityManager();

    void begin();

    bool checkWifiConnection();

    bool initWifi(const char* ssid, const char* password);

    //void turnOnWifi();

    void turnOffWifi();

    bool checkMqttConnection();

    bool initMqtt(const char* address, int port, const char* user, const char* password, const char* clientID);

    MQTTClient* getMqttClient();

private:

    WiFiClientSecure mWifiClientSecure;
    MQTTClient mMqttClient;

    const char* mMqttUser;
    const char* mMqttPassword;
    const char* mMqttClientID;

    SemaphoreHandle_t mWifiMutex = NULL;
    SemaphoreHandle_t mMqttMutex = NULL;

};

#endif /*CONNECTIVITY_MANAGER_H*/
