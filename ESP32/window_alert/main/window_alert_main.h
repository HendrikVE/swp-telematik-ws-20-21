#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_event_loop.h"

#define LOW 0
#define HIGH 1

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

void init_window_sensor();
void init_nvs();
void init_wifi();

void app_main();
