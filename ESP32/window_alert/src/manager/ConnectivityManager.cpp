#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "Arduino.h"
#include "ArduinoLog.h"
#include "WiFiClientSecure.h"
#include "MQTT.h"
#include "../storage/FlashStorage.h"

#include "ConnectivityManager.h"

static constexpr const char* TAG = "ConnectivityManager";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

const int WIFI_CONNECTED_BIT = BIT0;

static esp_err_t eventHandler(void *ctx, system_event_t *event) {

    switch(event->event_id) {

        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;

        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED");
            break;

        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED");
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            if (s_retry_num < 20) {
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG,"retry to connect to the AP");
            }
            ESP_LOGI(TAG,"connect to the AP fail\n");
            break;

        default:
            break;
    }

    return ESP_OK;
}

ConnectivityManager::ConnectivityManager() {
    mWifiMutex = xSemaphoreCreateMutex();
    mMqttMutex = xSemaphoreCreateMutex();
}

void ConnectivityManager::begin() {

}

bool ConnectivityManager::checkWifiConnection() {

    if (xSemaphoreTake(mWifiMutex, (TickType_t) 10 ) == pdTRUE) {

        wifi_ap_record_t apRecord;

        if (esp_wifi_sta_get_ap_info(&apRecord) != ESP_OK) {
            ESP_LOGI(TAG, "Connect to WiFi...");

            int attempts = 0;
            while (esp_wifi_sta_get_ap_info(&apRecord) != ESP_OK) {

                attempts++;
                if (attempts >= 60) {
                    return false;
                }

                delay(500);
            }
            ESP_LOGI(TAG, "Connected");
        }

        xSemaphoreGive(mWifiMutex);
    }

    return true;
}

bool ConnectivityManager::initWifi(const char* ssid, const char* password) {

    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(eventHandler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_sta_config_t sta_config = {};
    strcpy((char*) sta_config.ssid, ssid);
    strcpy((char*) sta_config.password, password);

    wifi_config_t wifi_config = {};
    wifi_config.sta = sta_config;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    return checkWifiConnection();
}

/*void ConnectivityManager::turnOnWifi() {

}*/

void ConnectivityManager::turnOffWifi() {
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
}

bool ConnectivityManager::checkMqttConnection() {

    if (xSemaphoreTake(mMqttMutex, (TickType_t) 10 ) == pdTRUE) {

        if (!mMqttClient.connected()) {
            ESP_LOGI(TAG, "Connect to MQTT broker...");

            int attempts = 0;
            while (!mMqttClient.connect(this->mMqttClientID, this->mMqttUser, this->mMqttPassword)) {
                //connect has timeout set by mMqttClient.setOptions()

                attempts++;
                if (attempts >= 60) {
                    return false;
                }
            }
            ESP_LOGI(TAG, "Connected");
        }

        xSemaphoreGive(mMqttMutex);
    }

    return true;
}

bool ConnectivityManager::initMqtt(const char* address, int port, const char* user, const char* password, const char* clientID) {

    mWifiClientSecure.setCACert((char*) ca_crt_start);
    mWifiClientSecure.setCertificate((char*) client_crt_start);
    mWifiClientSecure.setPrivateKey((char*) client_key_start);

    this->mMqttUser = user;
    this->mMqttPassword = password;
    this->mMqttClientID = clientID;

    mMqttClient.setOptions(10, true, 500);
    mMqttClient.begin(address, port, mWifiClientSecure);

    return checkMqttConnection();
}

MQTTClient* ConnectivityManager::getMqttClient() {
    return &mMqttClient;
}
