#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/rtc_io.h"
#include "esp_sleep.h"

#include "MANIFEST.h"

#include "Arduino.h"
#include "Wire.h"
#include "math.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include "Adafruit_BME680.h"

#include "manager/ConnectivityManager.cpp"
#include "manager/UpdateManager.cpp"
#include "storage/FlashStorage.h"

#include "hardware/EnvironmentSensor.cpp"

struct WindowSensor {
    int id;
    int gpio_input;
    int gpio_output;
    int interrupt_debounce;
    char mqtt_topic[128];
    unsigned long timestamp_last_interrupt;
    char last_state;
} window_sensor_1, window_sensor_2;

struct WindowSensorEvent {
    WindowSensor* windowSensor;
    bool level;
};

EnvironmentSensor* environmentSensor;

void buildTopic(char* output, const char* room, const char* boardID, const char* measurement) {

    sprintf(output, "room/%s/%s/%s", room, boardID, measurement);
}

static boolean queuePaused = false;
static xQueueHandle windowSensorEventQueue = NULL;

ConnectivityManager connectivityManager;
UpdateManager updateManager;
MQTTClient mqttClient;

void IRAM_ATTR isrWindowSensor1() {

    if (queuePaused) return;

    WindowSensorEvent event;
    event.windowSensor = &window_sensor_1;
    event.level = digitalRead(window_sensor_1.gpio_input);

    xQueueSendFromISR(windowSensorEventQueue, &event, NULL);
}

void IRAM_ATTR isrWindowSensor2() {

    if (queuePaused) return;

    WindowSensorEvent event;
    event.windowSensor = &window_sensor_2;
    event.level = digitalRead(window_sensor_2.gpio_input);

    xQueueSendFromISR(windowSensorEventQueue, &event, NULL);
}

static void gpio_task_example(void* arg) {

    struct WindowSensor* window_sensor;
    WindowSensorEvent event;
    while (true) {
        if (xQueueReceive(windowSensorEventQueue, &event, portMAX_DELAY)) {

            connectivityManager.checkWiFiConnection();
            connectivityManager.checkMQTTConnection();

            window_sensor = event.windowSensor;
            bool current_state = event.level;

            unsigned long current_time = (unsigned long) esp_timer_get_time() / 1000;

            unsigned long time_diff = 0;

            if (current_time < window_sensor->timestamp_last_interrupt) {
                // catch overflow
                time_diff = window_sensor->interrupt_debounce + 1;
            }
            else {
                time_diff = current_time - window_sensor->timestamp_last_interrupt;
            }

            if (time_diff <= window_sensor->interrupt_debounce) {
                // not within debounce time -> ignore interrupt
                continue;
            }

            char output[128];
            sprintf(output, "window sensor #%i: ", window_sensor->id);
            Serial.print(output);

            if (current_state == LOW) {
                Serial.println("open");
                window_sensor->last_state = LOW;
                mqttClient.publish(window_sensor->mqtt_topic, "OPEN", false, 2);
            }
            else if (current_state == HIGH) {
                Serial.println("closed");
                window_sensor->last_state = HIGH;
                mqttClient.publish(window_sensor->mqtt_topic, "CLOSED", false, 2);
            }

            window_sensor->timestamp_last_interrupt = current_time;
        }
    }
}

void init_window_sensor(struct WindowSensor window_sensor, void (*isr)()) {

    pinMode(window_sensor.gpio_output, OUTPUT);
    pinMode(window_sensor.gpio_input, INPUT_PULLDOWN);

    attachInterrupt(digitalPinToInterrupt(window_sensor.gpio_input), isr, CHANGE);

    // output always on to detect changes on input
    digitalWrite(window_sensor.gpio_output, HIGH);
}

void configureWindowSensorSystem() {

    char topic[128];

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        Serial.println("configureWindowSensorSystem(1)");

        window_sensor_1.id = 1;
        window_sensor_1.gpio_input = CONFIG_SENSOR_WINDOW_1_GPIO_INPUT;
        window_sensor_1.gpio_output = CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT;
        window_sensor_1.interrupt_debounce = CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS;

        buildTopic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);
        strcpy(window_sensor_1.mqtt_topic, topic);

        window_sensor_1.timestamp_last_interrupt = 0;

        init_window_sensor(window_sensor_1, &isrWindowSensor1);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        Serial.println("configureWindowSensorSystem(2)");

        window_sensor_2.id = 2;
        window_sensor_2.gpio_input = CONFIG_SENSOR_WINDOW_2_GPIO_INPUT;
        window_sensor_2.gpio_output = CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT;
        window_sensor_2.interrupt_debounce = CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS;

        buildTopic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);
        strcpy(window_sensor_2.mqtt_topic, topic);

        window_sensor_2.timestamp_last_interrupt = 0;

        init_window_sensor(window_sensor_2, &isrWindowSensor2);
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

        detachInterrupt(digitalPinToInterrupt(window_sensor_1.gpio_input));

        gpio_num_t windowSensor1Input = (gpio_num_t) window_sensor_1.gpio_input;
        gpio_num_t windowSensor1Output = (gpio_num_t) window_sensor_1.gpio_output;

        rtc_gpio_init(windowSensor1Input);
        rtc_gpio_init(windowSensor1Output);

        rtc_gpio_set_direction(windowSensor1Input, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_set_direction(windowSensor1Output, RTC_GPIO_MODE_OUTPUT_ONLY);

        gpio_pullup_dis(windowSensor1Input);
        gpio_pulldown_en(windowSensor1Input);

        rtc_gpio_set_level(windowSensor1Output, HIGH);

        window_sensor_1.last_state = rtc_gpio_get_level(windowSensor1Input);

        if (window_sensor_1.last_state == LOW) {
            esp_sleep_enable_ext0_wakeup(windowSensor1Input, HIGH);
        }
        else if (window_sensor_1.last_state == HIGH) {
            esp_sleep_enable_ext0_wakeup(windowSensor1Input, LOW);
        }

        rtc_gpio_hold_en(windowSensor1Input);
        rtc_gpio_hold_en(windowSensor1Output);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

        detachInterrupt(digitalPinToInterrupt(window_sensor_2.gpio_input));

        gpio_num_t windowSensor2Input = (gpio_num_t) window_sensor_2.gpio_input;;
        gpio_num_t windowSensor2Output = (gpio_num_t) window_sensor_2.gpio_output;;

        rtc_gpio_init(windowSensor2Input);
        rtc_gpio_init(windowSensor2Output);

        rtc_gpio_set_direction(windowSensor2Input, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_set_direction(windowSensor2Output, RTC_GPIO_MODE_OUTPUT_ONLY);

        gpio_pullup_dis(windowSensor2Input);
        gpio_pulldown_en(windowSensor2Input);

        rtc_gpio_set_level(windowSensor2Output, HIGH);

        window_sensor_2.last_state = rtc_gpio_get_level(windowSensor2Input);

        if (window_sensor_2.last_state == LOW) {
            esp_sleep_enable_ext1_wakeup(BIT(windowSensor2Input), ESP_EXT1_WAKEUP_ANY_HIGH);
        }
        else if (window_sensor_2.last_state == HIGH) {
            esp_sleep_enable_ext1_wakeup(BIT(windowSensor2Input), ESP_EXT1_WAKEUP_ALL_LOW);
        }

        rtc_gpio_hold_en(windowSensor2Input);
        rtc_gpio_hold_en(windowSensor2Output);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/

    esp_sleep_enable_timer_wakeup(sleepIntervalMS * 1000L);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);

    // give a chance for serial prints
    delay(500);

    esp_deep_sleep_start();

    Serial.println("woke up");

    WiFi.mode(WIFI_STA);

    // RTC GPIO pins need to be reconfigured as digital GPIO after sleep
    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        rtc_gpio_deinit(windowSensor1Input);
        rtc_gpio_deinit(windowSensor1Output);

        init_window_sensor(window_sensor_1, &isrWindowSensor1);
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        rtc_gpio_deinit(windowSensor2Input);
        rtc_gpio_deinit(windowSensor2Output);

        init_window_sensor(window_sensor_2, &isrWindowSensor2);
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void setup(){

    Serial.begin(115200);

    connectivityManager.initWiFi();
    connectivityManager.initMQTT();
    mqttClient = *connectivityManager.get_mqttClient();

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

    connectivityManager.checkWiFiConnection();
    connectivityManager.checkMQTTConnection();

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
