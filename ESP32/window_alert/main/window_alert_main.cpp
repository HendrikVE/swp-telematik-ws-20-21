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

#include "hardware/EnvironmentSensor.cpp"
#include "hardware/WindowSensor.cpp"
#include "manager/ConnectivityManager.cpp"
#include "manager/UpdateManager.cpp"
#include "storage/FlashStorage.h"

EnvironmentSensor* environmentSensor;
WindowSensor* windowSensor1;
WindowSensor* windowSensor2;

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
    event.windowSensor = windowSensor1;
    event.level = digitalRead(windowSensor1->getInputGpio());

    xQueueSendFromISR(windowSensorEventQueue, &event, NULL);
}

void IRAM_ATTR isrWindowSensor2() {

    if (queuePaused) return;

    WindowSensorEvent event;
    event.windowSensor = windowSensor2;
    event.level = digitalRead(windowSensor2->getInputGpio());

    xQueueSendFromISR(windowSensorEventQueue, &event, NULL);
}

static void gpio_task_example(void* arg) {

    WindowSensor* windowSensor;
    WindowSensorEvent event;
    while (true) {
        if (xQueueReceive(windowSensorEventQueue, &event, portMAX_DELAY)) {

            connectivityManager.checkWifiConnection();
            connectivityManager.checkMqttConnection();

            windowSensor = event.windowSensor;
            bool current_state = event.level;

            unsigned long current_time = (unsigned long) esp_timer_get_time() / 1000;

            unsigned long time_diff = 0;

            if (current_time < windowSensor->getTimestampLastInterrupt()) {
                // catch overflow
                time_diff = windowSensor->getInterruptDebounce() + 1;
            }
            else {
                time_diff = current_time - windowSensor->getTimestampLastInterrupt();
            }

            if (time_diff <= windowSensor->getInterruptDebounce()) {
                // not within debounce time -> ignore interrupt
                continue;
            }

            char output[128];
            sprintf(output, "window sensor #%i: ", windowSensor->getId());
            Serial.print(output);

            if (current_state == LOW) {
                Serial.println("open");
                windowSensor->setLastState(LOW);
                mqttClient.publish(windowSensor->getMqttTopic(), "OPEN", false, 2);
            }
            else if (current_state == HIGH) {
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

        windowSensor1 = new WindowSensor(
                CONFIG_SENSOR_WINDOW_1_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS,
                topic);

        windowSensor1->initGpio(&isrWindowSensor1);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

        buildTopic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);

        windowSensor2 = new WindowSensor(
                CONFIG_SENSOR_WINDOW_2_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS,
                topic);

        windowSensor2->initGpio(&isrWindowSensor2);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void initWindowSensorSystem() {

    Serial.println("init task queue");
    windowSensorEventQueue = xQueueCreate(10, sizeof(struct WindowSensorEvent));
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    configureWindowSensorSystem();
}


void publishEnvironmentData() {

    float temperature(NAN), humidity(NAN), pressure(NAN);

    temperature = environmentSensor->readTemperature();
    humidity = environmentSensor->readHumidity();
    pressure = environmentSensor->readPressure();


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
        if (environmentSensor->supportingGasResistence()) {

            float gas_resistance(NAN);
            gas_resistance = environmentSensor->readGasResistence();

            char strGasResistence[32];
            sprintf(strGasResistence, "%d", (int) round(gas_resistance));

            Serial.print("gas resistence: ");
            Serial.println(strGasResistence);

            char topicGas[128];
            buildTopic(topicGas, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_GAS);

            mqttClient.publish(topicGas, strGasResistence, false, 2);
        }
    #endif /*CONFIG_SENSOR_BME_680*/
}

void startDeviceSleep(int sleepIntervalMS) {

    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

        detachInterrupt(digitalPinToInterrupt(windowSensor1->getInputGpio()));

        gpio_num_t windowSensor1Input = (gpio_num_t) windowSensor1->getInputGpio();
        gpio_num_t windowSensor1Output = (gpio_num_t) windowSensor1->getOutputGpio();

        windowSensor1->initRtcGpio();

        windowSensor1->setLastState(rtc_gpio_get_level(windowSensor1Input));

        if (windowSensor1->getLastState() == LOW) {
            esp_sleep_enable_ext0_wakeup(windowSensor1Input, HIGH);
        }
        else if (windowSensor1->getLastState() == HIGH) {
            esp_sleep_enable_ext0_wakeup(windowSensor1Input, LOW);
        }

        rtc_gpio_hold_en(windowSensor1Input);
        rtc_gpio_hold_en(windowSensor1Output);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

        detachInterrupt(digitalPinToInterrupt(windowSensor2->getInputGpio()));

        gpio_num_t windowSensor2Input = (gpio_num_t) windowSensor2->getInputGpio();
        gpio_num_t windowSensor2Output = (gpio_num_t) windowSensor2->getOutputGpio();

        windowSensor2->initRtcGpio();

        windowSensor2->setLastState(rtc_gpio_get_level(windowSensor2Input));

        if (windowSensor2->getLastState() == LOW) {
            esp_sleep_enable_ext1_wakeup(BIT(windowSensor2Input), ESP_EXT1_WAKEUP_ANY_HIGH);
        }
        else if (windowSensor2->getLastState() == HIGH) {
            esp_sleep_enable_ext1_wakeup(BIT(windowSensor2Input), ESP_EXT1_WAKEUP_ALL_LOW);
        }

        rtc_gpio_hold_en(windowSensor2Input);
        rtc_gpio_hold_en(windowSensor2Output);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/

    esp_sleep_enable_timer_wakeup(sleepIntervalMS * 1000L);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);

    // give a chance for serial prints
    delay(500);

    esp_deep_sleep_start();

    Serial.println("woke up");

    WiFi.mode(WIFI_STA);

    // RTC GPIO pins need to be reconfigured as digital GPIO after sleep
    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        windowSensor1->deinitRtcGpio();

        windowSensor1->initGpio(&isrWindowSensor1);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        windowSensor2->deinitRtcGpio();

        windowSensor2->initGpio(&isrWindowSensor2);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void setup(){

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

    #if CONFIG_SENSOR_BME_280
        environmentSensor = new EnvironmentSensor(Sensor::BME280);
    #endif /*CONFIG_SENSOR_BME_280*/

    #if CONFIG_SENSOR_BME_680
        environmentSensor = new EnvironmentSensor(Sensor::BME680);
    #endif /*CONFIG_SENSOR_BME_680*/

    environmentSensor->init();
}

void loop(){

    Serial.println("loop");

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

    queuePaused = false;

    publishEnvironmentData();

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
