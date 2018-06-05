#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_event_loop.h"

#define LOW 0
#define HIGH 1

#define ESP_INTR_FLAG_DEFAULT 0

extern const uint8_t iot_eclipse_org_pem_start[] asm("_binary_iot_eclipse_org_pem_start");
extern const uint8_t iot_eclipse_org_pem_end[] asm("_binary_iot_eclipse_org_pem_end");

struct WindowSensor {
    int gpio_input;
    int gpio_output;
    int interrupt_debounce;
    char mqtt_topic[128];
    unsigned long timestamp_last_interrupt;
};

static void IRAM_ATTR gpio_isr_handler(void* arg);
static void gpio_task_example(void* arg);

static esp_err_t event_handler(void *ctx, system_event_t *event);

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static void mqtt_app_start(void);

void init_window_sensor(struct WindowSensor window_sensor);
void init_nvs();

void println();

void app_main();
