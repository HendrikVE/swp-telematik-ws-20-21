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

#include "window_alert_main.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static xQueueHandle gpio_evt_queue = NULL;

static esp_err_t event_handler(void *ctx, system_event_t *event) {

    switch(event->event_id) {

        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;

        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                     MAC2STR(event->event_info.sta_connected.mac),
                     event->event_info.sta_connected.aid);
            break;

        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                     MAC2STR(event->event_info.sta_disconnected.mac),
                     event->event_info.sta_disconnected.aid);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;

        default:
            break;
    }

    return ESP_OK;
}

void wifi_init_softap() {

    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = BOARD_WIFI_SSID,
            .ssid_len = strlen(BOARD_WIFI_SSID),
            .password = BOARD_WIFI_PASS,
            .max_connection = BOARD_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(BOARD_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s", BOARD_WIFI_SSID);
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {

    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg) {

    unsigned long timestamp_last_interrupt = 0;

    uint32_t io_num;
    while (true) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {

            unsigned long current_time = (unsigned long) esp_timer_get_time() / 1000;

            unsigned long time_diff = 0;

            if (current_time < timestamp_last_interrupt) {
                // catch overflow
                time_diff = WINDOW_INTERRUPT_DEBOUNCE_MS + 1;
            }
            else {
                time_diff = current_time - timestamp_last_interrupt;
            }

            if (time_diff <= WINDOW_INTERRUPT_DEBOUNCE_MS) {
                // not within debounce time -> ignore interrupt
                continue;
            }

            if (gpio_get_level(io_num) == LOW) {
                println("window has been opened");
            }
            else if (gpio_get_level(io_num) == HIGH) {
                println("window has been closed");
            }

            timestamp_last_interrupt = current_time;
        }
    }
}

void init_magnetic_sensor() {

	set_gpio_output(GPIO_OUTPUT_MAGNETIC_SENSOR);
	set_gpio_input(GPIO_INPUT_MAGNETIC_SENSOR, true, false, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_INPUT_MAGNETIC_SENSOR, gpio_isr_handler, (void*) GPIO_INPUT_MAGNETIC_SENSOR);

    // output always on to detect changes on input
    gpio_set_level(GPIO_OUTPUT_MAGNETIC_SENSOR, HIGH);

}

void set_gpio_output(int gpio_pin) {

	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = (1ULL << gpio_pin);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

void set_gpio_input(int gpio_pin, bool pull_down, bool pull_up, gpio_int_type_t intr_type) {

	gpio_config_t io_conf;
	//interrupt of rising edge
    io_conf.intr_type = intr_type;
    //bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << gpio_pin);
    io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_down_en = pull_down;
    io_conf.pull_up_en = pull_up;
    gpio_config(&io_conf);
}

void wifi_init_sta() {

    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = BOARD_WIFI_SSID,
            .password = BOARD_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s", BOARD_WIFI_SSID);
}

void init_nvs() {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void init_wifi() {

    #if BOARD_WIFI_MODE_AP
        ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
        wifi_init_softap();
    #else
        ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
        wifi_init_sta();
    #endif /*BOARD_WIFI_MODE_AP*/
}

void println(char* text) {
    printf("%s\n", text);
}

void app_main() {

    ESP_LOGI(TAG, "init_nvs()");
    init_nvs();

    ESP_LOGI(TAG, "init_wifi()");
    init_wifi();

    ESP_LOGI(TAG, "init_magnetic_sensor()");
    init_magnetic_sensor();

    // initial fake interrupt
    int gpio_pin = GPIO_OUTPUT_MAGNETIC_SENSOR;
    gpio_isr_handler(&gpio_pin);
}
