#include <stdbool.h>

#include "Arduino.h"
#include "ArduinoLog.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"
#include "../storage/FlashStorage.h"

class ConnectivityManager {

public:

    ConnectivityManager() {
        mWifiMutex = xSemaphoreCreateMutex();
        mMqttMutex = xSemaphoreCreateMutex();
    }

    void begin() {
        logger.begin(LOG_LEVEL_VERBOSE, &Serial);
        logger.setPrefix(printTag);
        logger.setSuffix(printNewline);
    }

    bool checkWifiConnection() {

        if (xSemaphoreTake(mWifiMutex, (TickType_t) 10 ) == pdTRUE) {

            if (WiFi.status() != WL_CONNECTED) {
                logger.notice("Connect to WiFi...");

                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED) {

                    attempts++;
                    if (attempts >= 60) {
                        return false;
                    }

                    delay(500);
                }
                logger.notice("Connected");
            }

            xSemaphoreGive(mWifiMutex);
        }

        return true;
    }

    bool initWifi() {
        //WiFi.onEvent(WiFiEvent);
        WiFi.begin(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);

        return checkWifiConnection();
    }

    void turnOnWifi() {
        WiFi.mode(WIFI_STA);
    }

    void turnOffWifi() {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
    }

    bool checkMqttConnection() {

        if (xSemaphoreTake(mMqttMutex, (TickType_t) 10 ) == pdTRUE) {

            if (!mMqttClient.connected()) {
                logger.notice("Connect to MQTT broker...");

                int attempts = 0;
                while (!mMqttClient.connect(CONFIG_DEVICE_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD)) {
                    //connect has timeout set by mMqttClient.setOptions()

                    attempts++;
                    if (attempts >= 60) {
                        return false;
                    }
                }
                logger.notice("Connected");
            }

            xSemaphoreGive(mMqttMutex);
        }

        return true;
    }

    bool initMqtt() {

        mWifiClientSecure.setCACert((char*) ca_crt_start);
        mWifiClientSecure.setCertificate((char*) client_crt_start);
        mWifiClientSecure.setPrivateKey((char*) client_key_start);

        mMqttClient.setOptions(10, true, 500);
        mMqttClient.begin(CONFIG_MQTT_SERVER_IP, CONFIG_MQTT_SERVER_PORT, mWifiClientSecure);

        return checkMqttConnection();
    }

    MQTTClient* getMqttClient() {
        return &mMqttClient;
    }

private:

    WiFiClientSecure mWifiClientSecure;
    MQTTClient mMqttClient;

    Logging logger;

    SemaphoreHandle_t mWifiMutex = NULL;
    SemaphoreHandle_t mMqttMutex = NULL;

    void static WiFiEvent(WiFiEvent_t event) {

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
