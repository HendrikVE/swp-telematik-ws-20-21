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
static esp_mqtt_client_handle_t client = NULL;

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

void printBME280Data(Stream* client) {

    float temp(NAN), hum(NAN), pres(NAN);

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(pres, temp, hum, tempUnit, presUnit);

    client->print("Temp: ");
    client->print(temp);
    client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
    client->print("\t\tHumidity: ");
    client->print(hum);
    client->print("% RH");
    client->print("\t\tPressure: ");
    client->print(pres);
    client->println("Pa");

    delay(1000);
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

            Serial.print("windowSensorNum = ");
            Serial.print(windowSensorNum);
            Serial.print("\n");

            if (digitalRead(window_sensor->gpio_input) == LOW) {
                Serial.println("open");
                esp_mqtt_client_publish(client, window_sensor->mqtt_topic, "OPEN", 0, 1, 0);
            }
            else if (digitalRead(window_sensor->gpio_input) == HIGH) {
                Serial.println("closed");
                esp_mqtt_client_publish(client, window_sensor->mqtt_topic, "CLOSED", 0, 1, 0);
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

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {

    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {

        case MQTT_EVENT_CONNECTED:
            Serial.println("MQTT_EVENT_CONNECTED");
            initWindowSensorSystem();
            initBME();

            break;

        case MQTT_EVENT_DISCONNECTED:
            Serial.println("MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            Serial.println("MQTT_EVENT_SUBSCRIBED");
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            Serial.println("MQTT_EVENT_UNSUBSCRIBED");
            break;

        case MQTT_EVENT_PUBLISHED:
            Serial.println("MQTT_EVENT_PUBLISHED");
            break;

        case MQTT_EVENT_DATA:
            Serial.println("MQTT_EVENT_DATA");
            break;

        case MQTT_EVENT_ERROR:
            Serial.println("MQTT_EVENT_ERROR");
            break;
    }

    return ESP_OK;
}

static void mqtt_app_start(void) {

    char server_uri[128];
    strcpy(server_uri, "mqtt://");
    strcat(server_uri, CONFIG_MQTT_SERVER_IP);
    strcat(server_uri, ":");
    strcat(server_uri, CONFIG_MQTT_SERVER_PORT);

    /*const esp_mqtt_client_config_t mqtt_cfg = {
            .uri = server_uri,
            .event_handle = mqtt_event_handler,
            .username = CONFIG_MQTT_USER,
            .password = CONFIG_MQTT_PASSWORD,
            //.cert_pem = (const char *)iot_eclipse_org_pem_start,
    };*/

    // we need to fill out all unused fields with default values
    // (error: "sorry, unimplemented: non-trivial designated initializers not supported")
    const esp_mqtt_client_config_t mqtt_cfg = {
            mqtt_event_handler,//mqtt_event_callback_t event_handle;
            nullptr,//const char *host;
            server_uri,//const char *uri;
            (uint32_t) CONFIG_MQTT_SERVER_PORT,//uint32_t port;
            "ESP",//const char *client_id;
            CONFIG_MQTT_USER,//const char *username;
            CONFIG_MQTT_PASSWORD,//const char *password;
            NULL,//const char *lwt_topic;
            NULL,//const char *lwt_msg;
            NULL,//int lwt_qos;
            NULL,//int lwt_retain;
            NULL,//int lwt_msg_len;
            true,//int disable_clean_session;
            120,//int keepalive;
            false,//bool disable_auto_reconnect;
            nullptr,//void *user_context;
            5,//int task_prio;
            6144,//int task_stack;
            1024,//int buffer_size;
            NULL,//const char *cert_pem;
            esp_mqtt_transport_t {},// esp_mqtt_transport_t transport;
            //.cert_pem = (const char *)iot_eclipse_org_pem_start,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void setup(){

    Serial.begin(115200);

    initWiFi();
    mqtt_app_start();
}

void loop(){

    checkWiFiConnection();

    delay(1000);
}
