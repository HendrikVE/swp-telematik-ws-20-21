#include <stdbool.h>

#include "Arduino.h"
#include "ArduinoLog.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"
#include "../storage/FlashStorage.h"

#include "ConnectivityManager.h"

ConnectivityManager::ConnectivityManager() {
    mWifiMutex = xSemaphoreCreateMutex();
    mMqttMutex = xSemaphoreCreateMutex();
}

void ConnectivityManager::begin() { }

bool ConnectivityManager::checkWifiConnection() {

    if (xSemaphoreTake(mWifiMutex, (TickType_t) 10 ) == pdTRUE) {

        if (WiFi.status() != WL_CONNECTED) {
            Log.notice("Connect to WiFi...");

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED) {

                attempts++;
                if (attempts >= 60) {
                    return false;
                }

                delay(500);
            }
            Log.notice("Connected");
        }

        xSemaphoreGive(mWifiMutex);
    }

    return true;
}

bool ConnectivityManager::initWifi(const char* ssid, const char* password) {
    // WiFi.onEvent(wifiEvent);
    WiFi.begin(ssid, password);

    mWifiClientSecure.setCACert((char*) ca_crt_start);
    mWifiClientSecure.setCertificate((char*) client_crt_start);
    mWifiClientSecure.setPrivateKey((char*) client_key_start);

    return checkWifiConnection();
}

void ConnectivityManager::turnOnWifi() {
    WiFi.mode(WIFI_STA);
}

void ConnectivityManager::turnOffWifi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

bool ConnectivityManager::checkMqttConnection() {

    if (xSemaphoreTake(mMqttMutex, (TickType_t) 10 ) == pdTRUE) {

        if (!mMqttClient.connected()) {
            Log.notice("Connect to MQTT broker...");

            int attempts = 0;
            while (!mMqttClient.connect(this->mMqttClientID, this->mMqttUser, this->mMqttPassword)) {
                // connect has timeout set by mMqttClient.setOptions()

                attempts++;
                if (attempts >= 60) {
                    return false;
                }
            }
            Log.notice("Connected");
        }

        xSemaphoreGive(mMqttMutex);
    }

    return true;
}

bool ConnectivityManager::initMqtt(
        const char* address, int port,
        const char* user, const char* password,
        const char* clientID) {

    this->mMqttUser = user;
    this->mMqttPassword = password;
    this->mMqttClientID = clientID;

    int keep_alive_ms = 5 * 60 * 1000; // keep connection alive while doing OTA updates
    int timeout_ms = 60 * 1000;

    mMqttClient.setOptions(keep_alive_ms, true, timeout_ms);
    mMqttClient.begin(address, port, mWifiClientSecure);

    return checkMqttConnection();
}

MQTTClient* ConnectivityManager::getMqttClient() {
    return &mMqttClient;
}
