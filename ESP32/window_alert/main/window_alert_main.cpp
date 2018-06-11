#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "mqtt_client.h"

#include "Arduino.h"
#include "WiFi.h"
#include "MQTT.h"
#include "BME280I2C.h"
#include "Wire.h"

extern const uint8_t iot_eclipse_org_pem_start[] asm("_binary_iot_eclipse_org_pem_start");
extern const uint8_t iot_eclipse_org_pem_end[] asm("_binary_iot_eclipse_org_pem_end");

struct WindowSensor {
    int gpio_input;
    int gpio_output;
    int interrupt_debounce;
    char mqtt_topic[128];
    unsigned long timestamp_last_interrupt;
} window_sensor_1, window_sensor_2;

static xQueueHandle gpio_evt_queue = NULL;

WiFiClient net;
MQTTClient client;

BME280I2C bme;

void WiFiEvent(WiFiEvent_t event) {

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

void checkWiFiConnection() {

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Trying to connect to WiFi...");
        WiFi.begin(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
        delay(10000);
    }
}

void initWiFi() {

    WiFi.onEvent(WiFiEvent);
    checkWiFiConnection();
}

void initBME() {

    Wire.begin();

    while(!bme.begin()) {
        Serial.println("Could not find BME280 sensor!");
        delay(1000);
    }
}

void IRAM_ATTR isrWindowSensor1() {
    uint32_t windowSensorNum = 1;
    xQueueSendFromISR(gpio_evt_queue, &windowSensorNum, NULL);
}

void IRAM_ATTR isrWindowSensor2() {
    uint32_t windowSensorNum = 2;
    xQueueSendFromISR(gpio_evt_queue, &windowSensorNum, NULL);
}

static void gpio_task_example(void* arg) {

    struct WindowSensor* window_sensor;
    uint32_t windowSensorNum;
    while (true) {
        if (xQueueReceive(gpio_evt_queue, &windowSensorNum, portMAX_DELAY)) {

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
            sprintf(output, "window sensor #%i", windowSensorNum);
            Serial.print(output);

            if (digitalRead(window_sensor->gpio_input) == LOW) {
                Serial.println("open");
                client.publish(window_sensor->mqtt_topic, "OPEN", false, 2);
            }
            else if (digitalRead(window_sensor->gpio_input) == HIGH) {
                Serial.println("closed");
                client.publish(window_sensor->mqtt_topic, "CLOSED", false, 2);
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

void initWindowSensorSystem() {

    Serial.println("init task queue");
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        Serial.println("init_window_sensor(1)");

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
        Serial.println("init_window_sensor(2)");

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

void checkMQTTConnection() {

    if (!client.connected()) {
        Serial.println("Trying to connect to MQTT broker...");

        while (!client.connect("esp32", CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD)) {
            Serial.print(".");
            delay(1000);
        }
    }
}

void initMQTT() {

    client.begin(CONFIG_MQTT_SERVER_IP, (int) CONFIG_MQTT_SERVER_PORT, net);
    checkMQTTConnection();

    initWindowSensorSystem();

    #if CONFIG_SENSOR_BME280_ENABLED
        initBME();
    #endif /*CONFIG_SENSOR_BME280_ENABLED*/
}

void publishBME280Data() {

    float temperature(NAN), humidity(NAN), pressure(NAN);

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(pressure, temperature, humidity, tempUnit, presUnit);

    char strTemperature[32];
    sprintf(strTemperature, "%f °%s", temperature, String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F').c_str());

    char strHumidity[32];
    sprintf(strHumidity, "%f %% RH", humidity);

    char strPressure[32];
    sprintf(strPressure, "%f Pa", pressure);

    Serial.println("");
    Serial.print("temperature: ");
    Serial.println(strTemperature);

    Serial.print("humidity: ");
    Serial.println(strHumidity);

    Serial.print("pressure: ");
    Serial.println(strPressure);
    Serial.println("");

    client.publish(CONFIG_SENSOR_BME280_MQTT_TOPIC_TEMPERATURE, strTemperature, false, 2);
    client.publish(CONFIG_SENSOR_BME280_MQTT_TOPIC_HUMIDITY, strHumidity, false, 2);
    client.publish(CONFIG_SENSOR_BME280_MQTT_TOPIC_PRESSURE, strPressure, false ,2);
}

void setup(){

    Serial.begin(115200);

    initWiFi();
    initMQTT();
}

void loop(){

    client.loop();
    delay(10); // <- fixes some issues with WiFi stability

    checkWiFiConnection();
    checkMQTTConnection();

    #if CONFIG_SENSOR_BME280_ENABLED
        publishBME280Data();
    #endif /*CONFIG_SENSOR_BME280_ENABLED*/

    delay(1000);
}
