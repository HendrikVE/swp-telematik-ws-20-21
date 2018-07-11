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
#include "Wire.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"

#include "ConnectivityManager.cpp"

#define BME_280_I2C_ADDRESS 0x76

struct WindowSensor {
    int gpio_input;
    int gpio_output;
    int interrupt_debounce;
    char mqtt_topic[128];
    unsigned long timestamp_last_interrupt;
    char last_state;
} window_sensor_1, window_sensor_2;

static xQueueHandle windowSensorEventQueue = NULL;

ConnectivityManager connectivityManager;
MQTTClient mqttClient;

Adafruit_BME280 bme;

void initBME() {

    Wire.begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SDC_GPIO_PIN);

    while(!bme.begin(BME_280_I2C_ADDRESS, &Wire)) {
        Serial.println("Could not find BME280 sensor!");
        delay(1000);
    }
}

void IRAM_ATTR isrWindowSensor1() {
    uint32_t windowSensorNum = 1;
    xQueueSendFromISR(windowSensorEventQueue, &windowSensorNum, NULL);
}

void IRAM_ATTR isrWindowSensor2() {
    uint32_t windowSensorNum = 2;
    xQueueSendFromISR(windowSensorEventQueue, &windowSensorNum, NULL);
}

static void gpio_task_example(void* arg) {

    struct WindowSensor* window_sensor;
    uint32_t windowSensorNum;
    while (true) {
        if (xQueueReceive(windowSensorEventQueue, &windowSensorNum, portMAX_DELAY)) {

            while (WiFi.status() != WL_CONNECTED || !mqttClient.connected()) {
                delay(1000);
            }

            if (windowSensorNum == 1) {
                window_sensor = &window_sensor_1;
            }
            else if (windowSensorNum == 2) {
                window_sensor = &window_sensor_2;
            }
            else {
                // ignore unkown source
                continue;
            }

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
            sprintf(output, "window sensor #%i: ", windowSensorNum);
            Serial.print(output);

            char current_state = digitalRead(window_sensor->gpio_input);
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

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        Serial.println("configureWindowSensorSystem(1)");

        window_sensor_1.gpio_input = CONFIG_SENSOR_WINDOW_1_GPIO_INPUT;
        window_sensor_1.gpio_output = CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT;
        window_sensor_1.interrupt_debounce = CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS;
        strcpy(window_sensor_1.mqtt_topic, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);
        window_sensor_1.timestamp_last_interrupt = 0;

        init_window_sensor(window_sensor_1, &isrWindowSensor1);

        // initial fake interrupt
        isrWindowSensor1();
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        Serial.println("configureWindowSensorSystem(2)");

        window_sensor_2.gpio_input = CONFIG_SENSOR_WINDOW_2_GPIO_INPUT;
        window_sensor_2.gpio_output = CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT;
        window_sensor_2.interrupt_debounce = CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS;
        strcpy(window_sensor_2.mqtt_topic, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);
        window_sensor_2.timestamp_last_interrupt = 0;

        init_window_sensor(window_sensor_2, &isrWindowSensor2);

        // initial fake interrupt
        isrWindowSensor2();
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void initWindowSensorSystem() {

    Serial.println("init task queue");
    windowSensorEventQueue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    configureWindowSensorSystem();
}

void publishBME280Data() {

    float temperature(NAN), humidity(NAN), pressure(NAN);

    temperature = bme.readTemperature();
    humidity = bme.readPressure();
    pressure = bme.readHumidity();

    char strTemperature[512];
    sprintf(strTemperature, "%f", temperature);

    char strHumidity[512];
    sprintf(strHumidity, "%f", humidity);

    char strPressure[512];
    sprintf(strPressure, "%f", pressure);

    Serial.println("");
    Serial.print("temperature: ");
    Serial.println(strTemperature);

    Serial.print("humidity: ");
    Serial.println(strHumidity);

    Serial.print("pressure: ");
    Serial.println(strPressure);
    Serial.println("");

    mqttClient.publish(CONFIG_SENSOR_BME280_MQTT_TOPIC_TEMPERATURE, strTemperature, false, 2);
    mqttClient.publish(CONFIG_SENSOR_BME280_MQTT_TOPIC_HUMIDITY, strHumidity, false, 2);
    mqttClient.publish(CONFIG_SENSOR_BME280_MQTT_TOPIC_PRESSURE, strPressure, false, 2);
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
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);

    esp_light_sleep_start();

    Serial.println("woke up");

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

    initWindowSensorSystem();

    #if CONFIG_SENSOR_BME280_ENABLED
        initBME();
    #endif /*CONFIG_SENSOR_BME280_ENABLED*/

    mqttClient = connectivityManager.get_mqttClient();
}

void loop(){

    Serial.println("loop");

    mqttClient.loop();
    delay(10); // <- fixes some issues with WiFi stability

    connectivityManager.checkWiFiConnection();
    connectivityManager.checkMQTTConnection();

    #if CONFIG_SENSOR_BME280_ENABLED
        publishBME280Data();
    #endif /*CONFIG_SENSOR_BME280_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        isrWindowSensor1();
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        isrWindowSensor2();
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/

    // dont go to sleep before all tasks in queue are executed
    while (uxQueueMessagesWaiting(windowSensorEventQueue) > 0) {
        delay(1000);
    }

    Serial.println("go to sleep");
    startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
}
