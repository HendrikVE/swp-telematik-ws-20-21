#include "Arduino.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"
#include "../storage/FlashStorage.h"

class ConnectivityManager {

public:

    ConnectivityManager() {
        mWifiMutex = xSemaphoreCreateMutex();
        mMqttMutex = xSemaphoreCreateMutex();
    }

    void checkWifiConnection() {

        if (xSemaphoreTake(mWifiMutex, (TickType_t) 10 ) == pdTRUE) {

            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("Connect to WiFi...");
                WiFi.begin(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);

                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED) {

                    attempts++;
                    if (attempts >= 15) {
                        // restart device in case WiFi library wont connect
                        ESP.restart();
                    }

                    Serial.print(".");
                    delay(1000);
                }
            }

            xSemaphoreGive(mWifiMutex);
        }
    }

    void initWifi() {

        WiFi.onEvent(WiFiEvent);
        checkWifiConnection();
    }

    void turnOnWifi() {
        WiFi.mode(WIFI_STA);
    }

    void turnOffWifi() {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
    }

    void checkMqttConnection() {

        if (xSemaphoreTake(mMqttMutex, (TickType_t) 10 ) == pdTRUE) {

            if (!mMqttClient.connected()) {
                Serial.println("Connect to MQTT broker...");

                while (!mMqttClient.connect(CONFIG_DEVICE_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD)) {
                    Serial.print(".");
                    delay(1000);
                }
            }

            xSemaphoreGive(mMqttMutex);
        }
    }

    void initMqtt() {

        mWifiClientSecure.setCACert((char*) ca_crt_start);
        mWifiClientSecure.setCertificate((char*) client_crt_start);
        mWifiClientSecure.setPrivateKey((char*) client_key_start);

        mMqttClient.begin(CONFIG_MQTT_SERVER_IP, CONFIG_MQTT_SERVER_PORT, mWifiClientSecure);

        checkMqttConnection();
    }

    MQTTClient* getMqttClient() {
        return &mMqttClient;
    }

private:

    WiFiClientSecure mWifiClientSecure;
    MQTTClient mMqttClient;

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

};