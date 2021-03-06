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
#include "math.h"

#include "hardware/WindowSensor.h"
#include "manager/ConnectivityManager.h"
#include "manager/UpdateManager.h"
#include "storage/FlashStorage.h"

#ifndef CONFIG_SENSOR_NONE
    #include "hardware/EnvironmentSensor.h"
    EnvironmentSensor* pEnvironmentSensor;
#endif /*CONFIG_SENSOR_NONE*/

#ifndef CONFIG_BUILD_DEBUG
    #define DISABLE_LOGGING
#endif

WindowSensor *pWindowSensor1, *pWindowSensor2;

ConnectivityManager connectivityManager;
UpdateManager* updateManager;
MQTTClient mqttClient;

RTC_DATA_ATTR int bootCount = 0;

static boolean queuePaused = false;
static xQueueHandle windowSensorEventQueue = NULL;

void startDeviceSleep(uint64_t sleepIntervalMS);

void lazySetup();

void buildTopic(char* output, const char* boardID, const char* measurement) {

    sprintf(output, "myhome/%s/%s", boardID, measurement);
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
                Log.notice("Could not establish connection, back to sleep.");
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
            Log.notice(output);

            if (currentState == LOW) {
                Log.notice("open");
                windowSensor->setLastState(LOW);
                mqttClient.publish(windowSensor->getMqttTopic(), "OPEN", false, 2);
            }
            else if (currentState == HIGH) {
                Log.notice("closed");
                windowSensor->setLastState(HIGH);
                mqttClient.publish(windowSensor->getMqttTopic(), "CLOSED", false, 2);
            }

            windowSensor->setTimestampLastInterrupt(current_time);
        }
    }
}

#if CONFIG_MQTT_SPAM
void spam(const char* message)
{
    char topic[128];
    buildTopic(topic, CONFIG_MQTT_SPAM_DEVICE_ID, CONFIG_MQTT_SPAM_TOPIC);

    #if CONFIG_MQTT_SPAM_TROLL
        char msg[10];
        srand(time(0));
    #endif /*CONFIG_MQTT_SPAM_TROLL*/

    while(true)
    {
        //delay takes miliseconds
        delay(CONFIG_MQTT_SPAM_INTERVAL);
        #if CONFIG_MQTT_SPAM_TROLL
            if(rand() % 100 < 50)
            {
                sprintf(msg, "OPEN");
            }
            else{
                sprintf(msg, "CLOSED");
            }    
            Log.notice("Spamming %s to MqttTopic %s",msg,topic );
            mqttClient.publish(topic, msg, false, 2);
        #else
            Log.notice("Spamming %s to MqttTopic %s",message,topic );
            mqttClient.publish(topic, message, false, 2);
        #endif
    }
}
#endif /*CONFIG_MQTT_SPAM*/

void configureWindowSensorSystem() {

    char topic[128];

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

        buildTopic(topic, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);

        pWindowSensor1 = new WindowSensor(
                CONFIG_SENSOR_WINDOW_1_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor1->initGpio(&isrWindowSensor1);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

        buildTopic(topic, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);

        pWindowSensor2 = new WindowSensor(
                CONFIG_SENSOR_WINDOW_2_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor2->initGpio(&isrWindowSensor2);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void initWindowSensorSystem() {

    Log.notice("init task queue");
    windowSensorEventQueue = xQueueCreate(10, sizeof(struct WindowSensorEvent));
    xTaskCreate(windowSensorTask, "windowSensorTask", 2048, NULL, 10, NULL);

    configureWindowSensorSystem();
}

#ifndef CONFIG_SENSOR_NONE
void publishEnvironmentData() {

    if (!pEnvironmentSensor->isInitialized()) {
        Log.notice("Environment sensor is not initialized. Skip");
        return;
    }

    float temperature(NAN), humidity(NAN), pressure(NAN);

    humidity = pEnvironmentSensor->readHumidity();
    pressure = pEnvironmentSensor->readPressure();
    // don't read temperature first, as the value is in 50 % of executions -105 °C
    temperature = pEnvironmentSensor->readTemperature();


    char strTemperature[32];
    sprintf(strTemperature, "%.1f", round(temperature * 10.0) / 10.0);

    Log.notice("temperature: %s", strTemperature);

    char strHumidity[32];
    sprintf(strHumidity, "%d", (int) round(humidity));

    Log.notice("humidity: %s", strHumidity);


    char strPressure[32];
    sprintf(strPressure, "%d", (int) round(pressure));

    Log.notice("pressure: %s", strPressure);


    char topicTemperature[128];
    buildTopic(topicTemperature, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE);

    char topicHumidity[128];
    buildTopic(topicHumidity, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_HUMIDITY);

    char topicPressure[128];
    buildTopic(topicPressure, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_PRESSURE);

    mqttClient.publish(topicTemperature, strTemperature, false, 2);
    mqttClient.publish(topicHumidity, strHumidity, false, 2);
    mqttClient.publish(topicPressure, strPressure, false, 2);

    #if CONFIG_SENSOR_MQTT_TOPIC_GAS

        if (pEnvironmentSensor->supportingGasResistance()) {

            float gasResistance(NAN);
            gasResistance = pEnvironmentSensor->readGasResistance();

            char strGasResistence[32];
            sprintf(strGasResistence, "%d", (int) round(gasResistance));

            Log.notice("gas resistence: %s", strGasResistence);

            char topicGas[128];
            buildTopic(topicGas, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_GAS);

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

    int rc = updateManager->checkForOTAUpdate();
    if (rc < 0) {
        char topicError[128];
        buildTopic(topicError, CONFIG_DEVICE_ID, "error");

        if (rc == -UPDATE_ERROR_ACTIVATE) {
            mqttClient.publish("openhab/alarm/warning", "0", false, 2);
            mqttClient.publish(topicError, "(Warning) Unsigned OTA update denied", false, 2);
        }
    }
}

void handleWakeup(){

    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

    switch(wakeupReason) {

        case 1:
            Log.notice("Wakeup caused by external signal using RTC_IO");
            updateWindowState();
            break;

        case 2:
            Log.notice("Wakeup caused by external signal using RTC_CNTL");
            updateWindowState();
            break;

        case 3:
            Log.notice("Wakeup caused by timer");
            updateAll();
            break;

        case 4:
            Log.notice("Wakeup caused by touchpad");
            break;

        case 5:
            Log.notice("Wakeup caused by ULP program");
            break;

        default:
            Log.notice("Wakeup was not caused by deep sleep");
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

    Log.notice("woke up");

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
    buildTopic(topicVersion, CONFIG_DEVICE_ID, "version");

    mqttClient.publish(topicVersion, strVersion, true, 2);
    Log.notice("device is running version: %s", strVersion);

    updateManager = new UpdateManager();
    updateManager->begin(CONFIG_OTA_HOST, CONFIG_OTA_FILENAME);

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

    if (CONFIG_BUILD_DEBUG) {
        Serial.begin(115200);
        Log.begin(LOG_LEVEL_VERBOSE, &Serial);
        Log.setPrefix(printTag);
        Log.setSuffix(printNewline);
    }

    char macAddress[128];
    sprintf(macAddress, "This is my MAC: %s", WiFi.macAddress().c_str());
    Log.notice(macAddress);

    connectivityManager.begin();
    connectivityManager.initWifi(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    connectivityManager.initMqtt(CONFIG_MQTT_SERVER_IP, CONFIG_MQTT_SERVER_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD, CONFIG_DEVICE_ID);
    mqttClient = *connectivityManager.getMqttClient();

    if (strcmp(CONFIG_DEVICE_ID, "esp32-1") == 0) {
        char topicLocation[128];
        buildTopic(topicLocation, CONFIG_DEVICE_ID, "location");
        mqttClient.publish(topicLocation, "Basement", false, 2);

        char topicProtection[128];
        buildTopic(topicProtection, CONFIG_DEVICE_ID, "protection");
        mqttClient.publish(topicProtection, "Protected", false, 2);

        char topicBattery[128];
        buildTopic(topicBattery, CONFIG_DEVICE_ID, "battery");
        mqttClient.publish(topicBattery, "76", false, 2);
    }
    else if (strcmp(CONFIG_DEVICE_ID, "esp32-2") == 0) {
        char topicLocation[128];
        buildTopic(topicLocation, CONFIG_DEVICE_ID, "location");
        mqttClient.publish(topicLocation, "Groundfloor", false, 2);

        char topicProtection[128];
        buildTopic(topicProtection, CONFIG_DEVICE_ID, "protection");
        mqttClient.publish(topicProtection, "Not protected", false, 2);

        char topicBattery[128];
        buildTopic(topicBattery, CONFIG_DEVICE_ID, "battery");
        mqttClient.publish(topicBattery, "92", false, 2);
    }
    else if (strcmp(CONFIG_DEVICE_ID, "esp32-3") == 0) {
        char topicLocation[128];
        buildTopic(topicLocation, CONFIG_DEVICE_ID, "location");
        mqttClient.publish(topicLocation, "Firstfloor", false, 2);

        char topicBattery[128];
        buildTopic(topicBattery, CONFIG_DEVICE_ID, "battery");
        mqttClient.publish(topicBattery, "29", false, 2);
    }

    initWindowSensorSystem();
}

void loop() {

    queuePaused = false;

    mqttClient.loop();
    delay(10); // <- fixes some issues with WiFi stability

    bool successWiFi = connectivityManager.checkWifiConnection();
    bool successMqtt = connectivityManager.checkMqttConnection();

    if (!successWiFi || !successMqtt) {
        Log.notice("Could not establish connection, back to sleep.");
        startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
    }

    handleWakeup();

    //handle spam attack
    #if CONFIG_MQTT_SPAM
        #if CONFIG_MQTT_SPAM_CLOSED
            spam("CLOSED");
        #endif /*MQTT_CONFIG_SPAM_CLOSED*/
        #if CONFIG_MQTT_SPAM_OPEN
            spam("OPEN");
        #endif /*MQTT_CONFIG_SPAM_OPEN*/
        #if CONFIG_MQTT_SPAM_TROLL
            //parameter doesnt matter in this case
            spam("OPEN");
        #endif /*MQTT_CONFIG_SPAM_TROLL*/
    #endif /*CONFIG_MQTT_SPAM*/

    // dont go to sleep before all tasks in queue are executed
    while (uxQueueMessagesWaiting(windowSensorEventQueue) > 0) {
        delay(100);
    }
    queuePaused = true;

    Log.notice("go to sleep");
    startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
}
