#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_event_loop.h"

#define BOARD_WIFI_MODE_AP CONFIG_ESP_WIFI_MODE_AP //TRUE:AP FALSE:STA
#define BOARD_WIFI_SSID    CONFIG_ESP_WIFI_SSID
#define BOARD_WIFI_PASS    CONFIG_ESP_WIFI_PASSWORD
#define BOARD_MAX_STA_CONN CONFIG_MAX_STA_CONN

#define GPIO_OUTPUT_MAGNETIC_SENSOR 18
#define GPIO_INPUT_MAGNETIC_SENSOR  4

#define LOW 0
#define HIGH 1

#define WINDOW_INTERRUPT_DEBOUNCE_MS 100

#define ESP_INTR_FLAG_DEFAULT 0

extern const uint8_t iot_eclipse_org_pem_start[] asm("_binary_iot_eclipse_org_pem_start");
extern const uint8_t iot_eclipse_org_pem_end[] asm("_binary_iot_eclipse_org_pem_end");

static const char *TAG = "window alert";

void set_gpio_output(int gpio_pin);
void set_gpio_input(int gpio_pin, bool pull_down, bool pull_up, gpio_int_type_t intr_type);
static void IRAM_ATTR gpio_isr_handler(void* arg);
static void gpio_task_example(void* arg);

static esp_err_t event_handler(void *ctx, system_event_t *event);

void println();

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static void mqtt_app_start(void);

void wifi_init_softap();
void wifi_init_sta();

void init_magnetic_sensor();
void init_nvs();
void init_wifi();

void app_main();
