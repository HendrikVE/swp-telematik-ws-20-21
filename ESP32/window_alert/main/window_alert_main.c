#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"

#include "window_alert_main.h"
#include "common.h"
#include "gpio.h"
#include "wifi.h"

static xQueueHandle gpio_evt_queue = NULL;

static esp_mqtt_client_handle_t client = NULL;

struct WindowSensor window_sensor_1, window_sensor_2;

static void IRAM_ATTR gpio_isr_handler(void* arg) {

    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg) {

    struct WindowSensor* window_sensor;
    uint32_t gpio_num;
    while (true) {
        if (xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {

            if (gpio_num == window_sensor_1.gpio_input) {
                window_sensor = &window_sensor_1;
            }
            else if (gpio_num == window_sensor_2.gpio_input) {
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

            if (gpio_get_level(window_sensor->gpio_input) == LOW) {
                println("open");
                esp_mqtt_client_publish(client, window_sensor->mqtt_topic, "OPEN", 0, 1, 0);
            }
            else if (gpio_get_level(window_sensor->gpio_input) == HIGH) {
                println("closed");
                esp_mqtt_client_publish(client, window_sensor->mqtt_topic, "CLOSED", 0, 1, 0);
            }

            window_sensor->timestamp_last_interrupt = current_time;
        }
    }
}

void init_window_sensor(struct WindowSensor window_sensor) {

	set_gpio_output(window_sensor.gpio_output);
	set_gpio_input(window_sensor.gpio_input, true, false, GPIO_INTR_ANYEDGE);

    gpio_isr_handler_add(window_sensor.gpio_input, gpio_isr_handler, (void*) window_sensor.gpio_input);

    // output always on to detect changes on input
    gpio_set_level(window_sensor.gpio_output, HIGH);
}

void init_window_sensor_system() {

    ESP_LOGI(TAG, "init isr and task queue");
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    #if CONFIG_SENSOR_WINDOW_1_ENABLED
        ESP_LOGI(TAG, "init_window_sensor(1)");

        window_sensor_1.gpio_input = CONFIG_SENSOR_WINDOW_1_GPIO_INPUT;
        window_sensor_1.gpio_output = CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT;
        window_sensor_1.interrupt_debounce = CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS;
        strcpy(window_sensor_1.mqtt_topic, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);
        window_sensor_1.timestamp_last_interrupt = 0;

        init_window_sensor(window_sensor_1);

        // initial fake interrupt
        gpio_isr_handler(&(window_sensor_1.gpio_input));
    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED
        ESP_LOGI(TAG, "init_window_sensor(2)");

        window_sensor_2.gpio_input = CONFIG_SENSOR_WINDOW_2_GPIO_INPUT;
        window_sensor_2.gpio_output = CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT;
        window_sensor_2.interrupt_debounce = CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS;
        strcpy(window_sensor_2.mqtt_topic, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);
        window_sensor_2.timestamp_last_interrupt = 0;

        init_window_sensor(window_sensor_2);

        // initial fake interrupt
        gpio_isr_handler(&(window_sensor_2.gpio_input));
    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void init_nvs() {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {

    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            init_window_sensor_system();

            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
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

    const esp_mqtt_client_config_t mqtt_cfg = {
        //.uri = "mqtts://iot.eclipse.org:8883",
        .uri = server_uri,
        .event_handle = mqtt_event_handler,
        .username = CONFIG_MQTT_USER,
        .password = CONFIG_MQTT_PASSWORD,
        //.cert_pem = (const char *)iot_eclipse_org_pem_start,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void println(char* text) {
    printf("%s\n", text);
}

void app_main() {

    ESP_LOGI(TAG, "init_nvs()");
    init_nvs();

    ESP_LOGI(TAG, "init_wifi()");
    init_wifi();

    ESP_LOGI(TAG, "mqtt_app_start()");
    mqtt_app_start();
}
