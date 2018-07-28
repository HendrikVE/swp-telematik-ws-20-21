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
#include "WiFiClientSecure.h"
#include "Wire.h"
#include "math.h"
#include "Update.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include "Adafruit_BME680.h"

#include "HTTPClient.h"

#include "ConnectivityManager.cpp"

#define BME_280_I2C_ADDRESS 0x76
#define BME_680_I2C_ADDRESS 0x77

HTTPClient http;

RTC_DATA_ATTR int bootCount = 0;
int contentLength = 0;

void execOTA() {

    char request[256];
    sprintf(request, "https://%s/%s/%s/%s", (char*) CONFIG_OTA_HOST, (char*) CONFIG_DEVICE_ID, String(APP_VERSION_CODE + 1).c_str(), (char*) CONFIG_OTA_FILENAME);

    http.begin((char*) CONFIG_OTA_HOST, 443, request, (char*) ca_crt_start, (char*) client_crt_start, (char*) client_key_start);
    http.setAuthorization(CONFIG_OTA_SERVER_USERNAME, CONFIG_OTA_SERVER_PASSWORD);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.print("[HTTP] GET... failed, error: ");
        Serial.println(httpCode);

        Serial.println("Exiting OTA Update");

        http.end();

        return;
    }

    contentLength = http.getSize();
    Serial.println("Got " + String(contentLength) + " bytes from server");

    if (contentLength) {

        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            size_t written = Update.writeStream(*http.getStreamPtr());

            if (written == contentLength) {
                Serial.println("Written : " + String(written) + " successfully");
            }
            else {
                Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
            }

            if (Update.end()) {
                Serial.println("OTA done!");

                if (Update.isFinished()) {
                    Serial.println("Update successfully completed. Rebooting.");
                    ESP.restart();
                }
                else {
                    Serial.println("Update not finished? Something went wrong!");
                }
            }
            else {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
            }
        }
        else {
            Serial.println("Not enough space to begin OTA");
        }
    }
    else {
        Serial.println("There was no content in the response");
    }

    http.end();
}

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

void buildTopic(char* output, const char* room, const char* boardID, const char* measurement) {

    sprintf(output, "room/%s/%s/%s", room, boardID, measurement);
}

static boolean queuePaused = false;
static xQueueHandle windowSensorEventQueue = NULL;

ConnectivityManager connectivityManager;
MQTTClient mqttClient;

Adafruit_BME280 bme280;
Adafruit_BME680 bme680;

#if CONFIG_SENSOR_BME_280
void initBME280() {

    Wire.begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SDC_GPIO_PIN);

    while(!bme280.begin(BME_280_I2C_ADDRESS)) {
        Serial.println("Could not find BME280 sensor!");
        delay(1000);
    }

}
#endif /*CONFIG_SENSOR_BME_280*/

#if CONFIG_SENSOR_BME_680
void initBME680() {

    Wire.begin(CONFIG_I2C_SDA_GPIO_PIN, CONFIG_I2C_SDC_GPIO_PIN);

    while(!bme680.begin(BME_680_I2C_ADDRESS)) {
        Serial.println("Could not find BME680 sensor!");
        delay(1000);
    }
}
#endif /*CONFIG_SENSOR_BME_680*/

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

#if CONFIG_SENSOR_BME_280
void publishBME280Data() {

    float temperature(NAN), humidity(NAN), pressure(NAN);

    temperature = bme280.readTemperature();
    humidity = bme280.readHumidity();
    pressure = bme280.readPressure();

    char strTemperature[32];
    sprintf(strTemperature, "%.1f", round(temperature * 10.0) / 10.0);

    char strHumidity[32];
    sprintf(strHumidity, "%d", (int) round(humidity));

    char strPressure[32];
    sprintf(strPressure, "%d", (int) round(pressure));

    Serial.println("");
    Serial.print("temperature: ");
    Serial.println(strTemperature);

    Serial.print("humidity: ");
    Serial.println(strHumidity);

    Serial.print("pressure: ");
    Serial.println(strPressure);
    Serial.println("");

    char topicTemperature[128];
    buildTopic(topicTemperature, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE);

    char topicHumidity[128];
    buildTopic(topicHumidity, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_HUMIDITY);

    char topicPressure[128];
    buildTopic(topicPressure, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_PRESSURE);

    mqttClient.publish(topicTemperature, strTemperature, false, 2);
    mqttClient.publish(topicHumidity, strHumidity, false, 2);
    mqttClient.publish(topicPressure, strPressure, false, 2);
}
#endif /*CONFIG_SENSOR_BME_280*/

#if CONFIG_SENSOR_BME_680
void publishBME680Data() {

    if (!bme680.performReading()) {
        Serial.println("Failed to perform reading");
        return;
    }

    char strTemperature[32];
    sprintf(strTemperature, "%.1f", round(bme680.temperature * 10.0) / 10.0);

    char strHumidity[32];
    sprintf(strHumidity, "%d", (int) round(bme680.humidity));

    char strPressure[32];
    sprintf(strPressure, "%d", (int) round(bme680.pressure));

    char strGas[32];
    sprintf(strGas, "%d", (int) round(bme680.gas_resistance));

    Serial.println("");
    Serial.print("temperature: ");
    Serial.println(strTemperature);

    Serial.print("humidity: ");
    Serial.println(strHumidity);

    Serial.print("pressure: ");
    Serial.println(strPressure);
    Serial.println("");

    Serial.print("gas: ");
    Serial.println(strGas);
    Serial.println("");

    char topicTemperature[128];
    buildTopic(topicTemperature, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE);

    char topicHumidity[128];
    buildTopic(topicHumidity, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_HUMIDITY);

    char topicPressure[128];
    buildTopic(topicPressure, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_PRESSURE);

    char topicGas[128];
    buildTopic(topicGas, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_GAS);

    mqttClient.publish(topicTemperature, strTemperature, false, 2);
    mqttClient.publish(topicHumidity, strHumidity, false, 2);
    mqttClient.publish(topicPressure, strPressure, false, 2);
    mqttClient.publish(topicGas, strGas, false, 2);
}
#endif /*CONFIG_SENSOR_BME_680*/

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

    bootCount++;

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
        initBME280();
    #endif /*CONFIG_SENSOR_BME_280*/

    #if CONFIG_SENSOR_BME_680
        initBME680();
    #endif /*CONFIG_SENSOR_BME_680*/
}

void loop(){

    Serial.println("loop");
    execOTA();

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

    #if CONFIG_SENSOR_BME_280
        publishBME280Data();
    #endif /*CONFIG_SENSOR_BME_280*/

    #if CONFIG_SENSOR_BME_680
        publishBME680Data();
    #endif /*CONFIG_SENSOR_BME_680*/

    // dont go to sleep before all tasks in queue are executed
    while (uxQueueMessagesWaiting(windowSensorEventQueue) > 0) {
        delay(1000);
    }
    queuePaused = true;

    Serial.println("go to sleep");
    startDeviceSleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
}
