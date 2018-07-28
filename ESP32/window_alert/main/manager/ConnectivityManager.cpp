#include "Arduino.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"
#include "../storage/FlashStorage.h"

class ConnectivityManager {

private:

    WiFiClientSecure wiFiClientSecure;
    MQTTClient mqttClient;

    SemaphoreHandle_t wifiMutex = NULL;
    SemaphoreHandle_t mqttMutex = NULL;

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

public:

    ConnectivityManager() {
        wifiMutex = xSemaphoreCreateMutex();
        mqttMutex = xSemaphoreCreateMutex();
    }

    void checkWiFiConnection() {

        if (xSemaphoreTake(wifiMutex, (TickType_t) 10 ) == pdTRUE) {

            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("Connect to WiFi...");
                WiFi.begin(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
            }

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

            xSemaphoreGive(wifiMutex);
        }
    }

    void initWiFi() {

        WiFi.onEvent(WiFiEvent);
        checkWiFiConnection();
    }

    void checkMQTTConnection() {

        if (xSemaphoreTake(mqttMutex, (TickType_t) 10 ) == pdTRUE) {

            if (!mqttClient.connected()) {
                Serial.println("Connect to MQTT broker...");

                while (!mqttClient.connect(CONFIG_DEVICE_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD)) {
                    Serial.print(".");
                    delay(1000);
                }
            }

            xSemaphoreGive(mqttMutex);
        }
    }

    void initMQTT() {

        wiFiClientSecure.setCACert((char*) ca_crt_start);
        wiFiClientSecure.setCertificate((char*) client_crt_start);
        wiFiClientSecure.setPrivateKey((char*) client_key_start);

        mqttClient.begin(CONFIG_MQTT_SERVER_IP, CONFIG_MQTT_SERVER_PORT, wiFiClientSecure);

        checkMQTTConnection();
    }

    MQTTClient* get_mqttClient() {
        return &mqttClient;
    }

};