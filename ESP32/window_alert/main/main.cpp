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
#include "math.h"

#include "hardware/WindowSensor.cpp"
#include "manager/ConnectivityManager.cpp"
#include "manager/UpdateManager.cpp"
#include "storage/FlashStorage.h"

#ifndef CONFIG_SENSOR_NONE
    #include "hardware/EnvironmentSensor.cpp"
    EnvironmentSensor* pEnvironmentSensor;
#endif /*CONFIG_SENSOR_NONE*/

WindowSensor *pWindowSensor1, *pWindowSensor2;

ConnectivityManager connectivityManager;
UpdateManager updateManager;
MQTTClient mqttClient;

static boolean queuePaused = false;
static xQueueHandle windowSensorEventQueue = NULL;

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

static void gpioTask(void* arg) {

    WindowSensor* windowSensor;
    WindowSensorEvent event;
    while (true) {
        if (xQueueReceive(windowSensorEventQueue, &event, portMAX_DELAY)) {

            connectivityManager.checkWifiConnection();
            connectivityManager.checkMqttConnection();

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
            Serial.print(output);

            if (currentState == LOW) {
                Serial.println("open");
                windowSensor->setLastState(LOW);
                mqttClient.publish(windowSensor->getMqttTopic(), "OPEN", false, 2);
            }
            else if (currentState == HIGH) {
                Serial.println("closed");
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

    Serial.println("init task queue");
    windowSensorEventQueue = xQueueCreate(10, sizeof(struct WindowSensorEvent));
    xTaskCreate(gpioTask, "gpioTask", 2048, NULL, 10, NULL);

    configureWindowSensorSystem();
}

#ifndef CONFIG_SENSOR_NONE
void publishEnvironmentData() {

    float temperature(NAN), humidity(NAN), pressure(NAN);

    temperature = pEnvironmentSensor->readTemperature();
    humidity = pEnvironmentSensor->readHumidity();
    pressure = pEnvironmentSensor->readPressure();


    char strTemperature[32];
    sprintf(strTemperature, "%.1f", round(temperature * 10.0) / 10.0);

    Serial.print("temperature: ");
    Serial.println(strTemperature);


    char strHumidity[32];
    sprintf(strHumidity, "%d", (int) round(humidity));

    Serial.print("humidity: ");
    Serial.println(strHumidity);


    char strPressure[32];
    sprintf(strPressure, "%d", (int) round(pressure));

    Serial.print("pressure: ");
    Serial.println(strPressure);


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

            Serial.print("gas resistence: ");
            Serial.println(strGasResistence);

            char topicGas[128];
            buildTopic(topicGas, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_GAS);

            mqttClient.publish(topicGas, strGasResistence, false, 2);
        }

    #endif /*CONFIG_SENSOR_MQTT_TOPIC_GAS*/
}
#endif /*CONFIG_SENSOR_NONE*/

void startDeviceSleep(uint64_t sleepIntervalMS) {

    connectivityManager.turnOnWifi();

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
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);

    // give a chance for serial prints
    delay(500);

    #if CONFIG_DEEP_SLEEP
        esp_deep_sleep_start();
    #endif /*DEEP_SLEEP*/

    #if CONFIG_LIGHT_SLEEP
        esp_light_sleep_start();
    #endif /*LIGHT_SLEEP*/

    Serial.println("woke up");

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

void setup() {

    Serial.begin(115200);

    connectivityManager.initWifi();
    connectivityManager.initMqtt();
    mqttClient = *connectivityManager.getMqttClient();

    char strVersion[128];
    sprintf(strVersion, "%s (%i)", APP_VERSION_NAME, APP_VERSION_CODE);

    char topicVersion[128];
    buildTopic(topicVersion, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, "version");
    mqttClient.publish(topicVersion, strVersion, true, 2);

    Serial.println("device is running version: " + String(strVersion));

    initWindowSensorSystem();

    #ifndef CONFIG_SENSOR_NONE

        #if CONFIG_SENSOR_BME_280
            pEnvironmentSensor = new EnvironmentSensor(Sensor::BME280);
        #endif /*CONFIG_SENSOR_BME_280*/

        #if CONFIG_SENSOR_BME_680
            pEnvironmentSensor = new EnvironmentSensor(Sensor::BME680);
        #endif /*CONFIG_SENSOR_BME_680*/

        pEnvironmentSensor->init();

    #endif /*CONFIG_SENSOR_NONE*/
}

void loop() {

    queuePaused = false;

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        isrWindowSensor1();
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        isrWindowSensor2();
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/

    mqttClient.loop();
    delay(10); // <- fixes some issues with WiFi stability

    connectivityManager.checkWifiConnection();
    connectivityManager.checkMqttConnection();

    #ifndef CONFIG_SENSOR_NONE
        publishEnvironmentData();
    #endif /*CONFIG_SENSOR_NONE*/

    // dont go to sleep before all tasks in queue are executed
    while (uxQueueMessagesWaiting(windowSensorEventQueue) > 0) {
        delay(1000);
    }
    queuePaused = true;

    // after work is done, check for update before sleeping
    updateManager.checkForOTAUpdate();

    Serial.println("go to sleep");
    startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
}
