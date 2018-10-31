#include "MANIFEST.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/rtc_io.h"
#include "esp_sleep.h"

#include "Arduino.h"
#include "ArduinoLog.h"
#include "BLEUtils.h"
#include "math.h"

#include "hardware/WindowSensor.h"
#include "manager/ConnectivityManager.h"
#include "manager/UpdateManager.h"
#include "storage/FlashStorage.h"

#define STR_IMPL_(x) #x      //stringify argument
#define STR(x) STR_IMPL_(x)  //indirection to expand argument macros

#ifndef CONFIG_SENSOR_NONE
    #include "hardware/EnvironmentSensor.h"
    EnvironmentSensor* pEnvironmentSensor;
#endif /*CONFIG_SENSOR_NONE*/

#ifdef CONFIG_BUILD_DEBUG
    #define DEBUG true
#endif
#ifndef CONFIG_BUILD_DEBUG
    #define DEBUG false
    #define DISABLE_LOGGING
#endif

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

std::vector <std::vector <const char*>> CHARACTERISTICS = {
        {CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID, CONFIG_DEVICE_ROOM},
        {CHARACTERISTIC_CONFIG_DEVICE_ID_UUID, CONFIG_DEVICE_ID},
        {CHARACTERISTIC_CONFIG_OTA_HOST_UUID, CONFIG_OTA_HOST},
        {CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID, CONFIG_OTA_FILENAME},
        {CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID, CONFIG_OTA_SERVER_USERNAME},
        {CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID, CONFIG_OTA_SERVER_PASSWORD},
        {CHARACTERISTIC_CONFIG_WIFI_SSID_UUID, CONFIG_ESP_WIFI_SSID},
        {CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID, CONFIG_ESP_WIFI_PASSWORD},
        {CHARACTERISTIC_CONFIG_MQTT_USER_UUID, CONFIG_MQTT_USER},
        {CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID, CONFIG_MQTT_PASSWORD},
        {CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID, CONFIG_MQTT_SERVER_IP},
        {CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID, STR(CONFIG_MQTT_SERVER_PORT)},
        {CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID, STR(CONFIG_SENSOR_POLL_INTERVAL_MS)}
};

WindowSensor *pWindowSensor1, *pWindowSensor2;

ConnectivityManager connectivityManager;
UpdateManager* updateManager;
MQTTClient mqttClient;

Logging logger;

RTC_DATA_ATTR int bootCount = 0;

static boolean queuePaused = false;
static xQueueHandle windowSensorEventQueue = NULL;

void startDeviceSleep(uint64_t sleepIntervalMS);

void lazySetup();

void buildTopic(char* output, const char* room, const char* boardID, const char* measurement) {

    sprintf(output, "room/%s/%s/%s", room, boardID, measurement);
}

void IRAM_ATTR isrWindowSensor1() {

    if (queuePaused) return;

    WindowSensorEvent event;
    event.windowSensor = pWindowSensor1;
    event.level = digitalRead(pWindowSensor1->getInputGpio());

    xQueueSendFromISR(windowSensorEventQueue, &event, NULL);
}

void IRAM_ATTR isrWindowSensor2() {

    if (queuePaused) return;

    WindowSensorEvent event;
    event.windowSensor = pWindowSensor2;
    event.level = digitalRead(pWindowSensor2->getInputGpio());

    xQueueSendFromISR(windowSensorEventQueue, &event, NULL);
}

static void windowSensorTask(void* arg) {

    WindowSensor* windowSensor;
    WindowSensorEvent event;
    while (true) {
        if (xQueueReceive(windowSensorEventQueue, &event, portMAX_DELAY)) {

            bool successWiFi = connectivityManager.checkWifiConnection();
            bool successMqtt = connectivityManager.checkMqttConnection();

            if (!successWiFi || !successMqtt) {
                startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
            }

            windowSensor = event.windowSensor;
            bool currentState = event.level;

            unsigned long current_time = (unsigned long) esp_timer_get_time() / 1000;

            unsigned long timeDiff = 0;

            if (current_time < windowSensor->getTimestampLastInterrupt()) {
                // catch overflow
                timeDiff = windowSensor->getInterruptDebounce() + 1;
            }
            else {
                timeDiff = current_time - windowSensor->getTimestampLastInterrupt();
            }

            if (timeDiff <= windowSensor->getInterruptDebounce()) {
                // not within debounce time -> ignore interrupt
                continue;
            }

            char output[128];
            sprintf(output, "window sensor #%i: ", windowSensor->getId());
            logger.notice(output);

            if (currentState == LOW) {
                logger.notice("open");
                windowSensor->setLastState(LOW);
                mqttClient.publish(windowSensor->getMqttTopic(), "OPEN", false, 2);
            }
            else if (currentState == HIGH) {
                logger.notice("closed");
                windowSensor->setLastState(HIGH);
                mqttClient.publish(windowSensor->getMqttTopic(), "CLOSED", false, 2);
            }

            windowSensor->setTimestampLastInterrupt(current_time);
        }
    }
}

void configureWindowSensorSystem() {

    char topic[128];

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

        buildTopic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);

        pWindowSensor1 = new WindowSensor(
                CONFIG_SENSOR_WINDOW_1_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor1->initGpio(&isrWindowSensor1);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

        buildTopic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);

        pWindowSensor2 = new WindowSensor(
                CONFIG_SENSOR_WINDOW_2_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor2->initGpio(&isrWindowSensor2);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void initWindowSensorSystem() {

    logger.notice("init task queue");
    windowSensorEventQueue = xQueueCreate(10, sizeof(struct WindowSensorEvent));
    xTaskCreate(windowSensorTask, "windowSensorTask", 2048, NULL, 10, NULL);

    configureWindowSensorSystem();
}

#ifndef CONFIG_SENSOR_NONE
void publishEnvironmentData() {

    if (!pEnvironmentSensor->isInitialized()) {
        logger.notice("Environment sensor is not initialized. Skip");
        return;
    }

    float temperature(NAN), humidity(NAN), pressure(NAN);

    humidity = pEnvironmentSensor->readHumidity();
    pressure = pEnvironmentSensor->readPressure();
    // don't read temperature first, as the value is in 50 % of executions -105 °C
    temperature = pEnvironmentSensor->readTemperature();


    char strTemperature[32];
    sprintf(strTemperature, "%.1f", round(temperature * 10.0) / 10.0);

    logger.notice("temperature: %s", strTemperature);

    char strHumidity[32];
    sprintf(strHumidity, "%d", (int) round(humidity));

    logger.notice("humidity: %s", strHumidity);


    char strPressure[32];
    sprintf(strPressure, "%d", (int) round(pressure));

    logger.notice("pressure: %s", strPressure);


    char topicTemperature[128];
    buildTopic(topicTemperature, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE);

    char topicHumidity[128];
    buildTopic(topicHumidity, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_HUMIDITY);

    char topicPressure[128];
    buildTopic(topicPressure, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_PRESSURE);

    mqttClient.publish(topicTemperature, strTemperature, false, 2);
    mqttClient.publish(topicHumidity, strHumidity, false, 2);
    mqttClient.publish(topicPressure, strPressure, false, 2);

    #if CONFIG_SENSOR_MQTT_TOPIC_GAS

        if (pEnvironmentSensor->supportingGasResistance()) {

            float gasResistance(NAN);
            gasResistance = pEnvironmentSensor->readGasResistance();

            char strGasResistence[32];
            sprintf(strGasResistence, "%d", (int) round(gasResistance));

            logger.notice("gas resistence: %s", strGasResistence);

            char topicGas[128];
            buildTopic(topicGas, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_GAS);

            mqttClient.publish(topicGas, strGasResistence, false, 2);
        }

    #endif /*CONFIG_SENSOR_MQTT_TOPIC_GAS*/
}
#endif /*CONFIG_SENSOR_NONE*/

void updateWindowState() {

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        isrWindowSensor1();
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        isrWindowSensor2();
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void updateAll() {

    lazySetup();

    updateWindowState();

    #ifndef CONFIG_SENSOR_NONE
        publishEnvironmentData();
    #endif /*CONFIG_SENSOR_NONE*/

    updateManager->checkForOTAUpdate();
}

void handleWakeup(){

    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

    switch(wakeupReason) {

        case 1:
            logger.notice("Wakeup caused by external signal using RTC_IO");
            updateWindowState();
            break;

        case 2:
            logger.notice("Wakeup caused by external signal using RTC_CNTL");
            updateWindowState();
            break;

        case 3:
            logger.notice("Wakeup caused by timer");
            updateAll();
            break;

        case 4:
            logger.notice("Wakeup caused by touchpad");
            break;

        case 5:
            logger.notice("Wakeup caused by ULP program");
            break;

        default:
            logger.notice("Wakeup was not caused by deep sleep");
            updateAll();
            break;
    }
}

void startDeviceSleep(uint64_t sleepIntervalMS) {

    connectivityManager.turnOffWifi();

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

        detachInterrupt(digitalPinToInterrupt(pWindowSensor1->getInputGpio()));

        gpio_num_t windowSensor1Input = (gpio_num_t) pWindowSensor1->getInputGpio();

        pWindowSensor1->initRtcGpio();

        pWindowSensor1->setLastState(rtc_gpio_get_level(windowSensor1Input));

        if (pWindowSensor1->getLastState() == LOW) {
            esp_sleep_enable_ext0_wakeup(windowSensor1Input, HIGH);
        }
        else if (pWindowSensor1->getLastState() == HIGH) {
            esp_sleep_enable_ext0_wakeup(windowSensor1Input, LOW);
        }
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

        detachInterrupt(digitalPinToInterrupt(pWindowSensor2->getInputGpio()));

        gpio_num_t windowSensor2Input = (gpio_num_t) pWindowSensor2->getInputGpio();

        pWindowSensor2->initRtcGpio();

        pWindowSensor2->setLastState(rtc_gpio_get_level(windowSensor2Input));

        if (pWindowSensor2->getLastState() == LOW) {
            esp_sleep_enable_ext1_wakeup(BIT(windowSensor2Input), ESP_EXT1_WAKEUP_ANY_HIGH);
        }
        else if (pWindowSensor2->getLastState() == HIGH) {
            esp_sleep_enable_ext1_wakeup(BIT(windowSensor2Input), ESP_EXT1_WAKEUP_ALL_LOW);
        }
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/

    esp_sleep_enable_timer_wakeup(sleepIntervalMS * 1000ULL);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);

    // give a chance for serial prints
    delay(500);

    if (!DEBUG) {
        #if CONFIG_DEEP_SLEEP
            esp_deep_sleep_start();
        #endif /*DEEP_SLEEP*/

        #if CONFIG_LIGHT_SLEEP
            esp_light_sleep_start();
        #endif /*LIGHT_SLEEP*/
    }
    else {
        delay(sleepIntervalMS);
    }

    logger.notice("woke up");

    connectivityManager.turnOnWifi();

    // RTC GPIO pins need to be reconfigured as digital GPIO after sleep
    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        pWindowSensor1->deinitRtcGpio();

        pWindowSensor1->initGpio(&isrWindowSensor1);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        pWindowSensor2->deinitRtcGpio();

        pWindowSensor2->initGpio(&isrWindowSensor2);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.println("*********");
            Serial.print("New value: ");
            for (int i = 0; i < value.length(); i++)
                Serial.print(value[i]);

            Serial.println();
            Serial.println("*********");
        }
    }
};

static void printTag(Print* _logOutput) {
    char c[12];
    sprintf(c, "%s ", "[MAIN] ");
    _logOutput->print(c);
}

static void printNewline(Print* _logOutput) {
    _logOutput->print("\n");
}

// lazy setup is only necessary if handleWakeup() calls updateAll()
void lazySetup() {

    char strVersion[128];
    sprintf(strVersion, "%s (%i)", APP_VERSION_NAME, APP_VERSION_CODE);

    char topicVersion[128];
    buildTopic(topicVersion, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, "version");

    mqttClient.publish(topicVersion, strVersion, true, 2);
    logger.notice("device is running version: %s", strVersion);

    updateManager = new UpdateManager();
    updateManager->begin(CONFIG_OTA_HOST, CONFIG_OTA_FILENAME, CONFIG_OTA_SERVER_USERNAME, CONFIG_OTA_SERVER_PASSWORD, CONFIG_DEVICE_ID);

    #ifndef CONFIG_SENSOR_NONE

        #if CONFIG_SENSOR_BME_280
            pEnvironmentSensor = new EnvironmentSensor(Sensor::BME280);
        #endif /*CONFIG_SENSOR_BME_280*/

        #if CONFIG_SENSOR_BME_680
            pEnvironmentSensor = new EnvironmentSensor(Sensor::BME680);
        #endif /*CONFIG_SENSOR_BME_680*/

        pEnvironmentSensor->begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SCL_GPIO_PIN);

    #endif /*CONFIG_SENSOR_NONE*/
}

void setup() {

    if (DEBUG) {
        Serial.begin(115200);
        logger.begin(LOG_LEVEL_VERBOSE, &Serial);
        logger.setPrefix(printTag);
        logger.setSuffix(printNewline);
    }

    connectivityManager.begin();

    connectivityManager.initBluetoothConfig(SERVICE_UUID, new MyCallbacks(), &CHARACTERISTICS);
    connectivityManager.initWifi(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    connectivityManager.initMqtt(CONFIG_MQTT_SERVER_IP, CONFIG_MQTT_SERVER_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD, CONFIG_DEVICE_ID);
    mqttClient = *connectivityManager.getMqttClient();

    initWindowSensorSystem();
}

void loop() {

    queuePaused = false;

    mqttClient.loop();
    delay(10); // <- fixes some issues with WiFi stability

    bool successWiFi = connectivityManager.checkWifiConnection();
    bool successMqtt = connectivityManager.checkMqttConnection();

    if (!successWiFi || !successMqtt) {
        startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
    }

    handleWakeup();

    // dont go to sleep before all tasks in queue are executed
    while (uxQueueMessagesWaiting(windowSensorEventQueue) > 0) {
        delay(100);
    }
    queuePaused = true;

    logger.notice("go to sleep");
    startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
}
