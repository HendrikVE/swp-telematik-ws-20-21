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
#include "nvs_flash.h"

#include "Arduino.h"
#include "ArduinoLog.h"
#include "math.h"

#include "hardware/WindowSensor.h"
#include "manager/ConnectivityManager.h"
#include "manager/UpdateManager.h"
#include "storage/FlashStorage.h"

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

void IRAM_ATTR isrWindowSensor1(void* arg) {

    if (queuePaused) return;

    WindowSensorEvent event;
    event.windowSensor = pWindowSensor1;
    event.level = digitalRead(pWindowSensor1->getInputGpio());

    xQueueSendFromISR(windowSensorEventQueue, &event, NULL);
}

void IRAM_ATTR isrWindowSensor2(void* arg) {

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
            bool successMqtt = true;//connectivityManager.checkMqttConnection();

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
                //mqttClient.publish(windowSensor->getMqttTopic(), "OPEN", false, 2);
            }
            else if (currentState == HIGH) {
                logger.notice("closed");
                windowSensor->setLastState(HIGH);
                //mqttClient.publish(windowSensor->getMqttTopic(), "CLOSED", false, 2);
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

    //mqttClient.publish(topicTemperature, strTemperature, false, 2);
    //mqttClient.publish(topicHumidity, strHumidity, false, 2);
    //mqttClient.publish(topicPressure, strPressure, false, 2);

    #if CONFIG_SENSOR_MQTT_TOPIC_GAS

        if (pEnvironmentSensor->supportingGasResistance()) {

            float gasResistance(NAN);
            gasResistance = pEnvironmentSensor->readGasResistance();

            char strGasResistence[32];
            sprintf(strGasResistence, "%d", (int) round(gasResistance));

            logger.notice("gas resistence: %s", strGasResistence);

            char topicGas[128];
            buildTopic(topicGas, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_GAS);

            //mqttClient.publish(topicGas, strGasResistence, false, 2);
        }

    #endif /*CONFIG_SENSOR_MQTT_TOPIC_GAS*/
}
#endif /*CONFIG_SENSOR_NONE*/

void updateWindowState() {

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        isrWindowSensor1(NULL);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        isrWindowSensor2(NULL);
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

    #if CONFIG_DEEP_SLEEP
        esp_deep_sleep_start();
    #endif /*DEEP_SLEEP*/

    #if CONFIG_LIGHT_SLEEP
        esp_light_sleep_start();
    #endif /*LIGHT_SLEEP*/

    logger.notice("woke up");

    //connectivityManager.turnOnWifi();

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

    //mqttClient.publish(topicVersion, strVersion, true, 2);
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

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    connectivityManager.begin();
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
    bool successMqtt = true;//connectivityManager.checkMqttConnection();

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
