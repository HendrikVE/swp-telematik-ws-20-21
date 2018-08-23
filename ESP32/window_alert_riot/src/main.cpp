#include "MANIFEST.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "saul_reg.h"
#include "xtimer.h"
#include "periph/gpio.h"
#include "msg.h"
#include "thread.h"

#include "window_sensor.h"

#define LOW 0
#define HIGH 1

#define RCV_QUEUE_SIZE (8)

struct window_sensor window_sensor_1, window_sensor_2;

static kernel_pid_t window_sensor_task_pid;
static char rcv_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
static msg_t rcv_queue[RCV_QUEUE_SIZE];


void build_topic(char *output, const char *room, const char *boardID, const char *measurement) {

    sprintf(output, "room/%s/%s/%s", room, boardID, measurement);
}

void publish_environment_data(void) {

    phydat_t temperature;
    phydat_t humidity;
    phydat_t pressure;

    saul_reg_t* devTemp = saul_reg_find_type(SAUL_SENSE_TEMP);
    saul_reg_t* devHum = saul_reg_find_type(SAUL_SENSE_HUM);
    saul_reg_t* devPres = saul_reg_find_type(SAUL_SENSE_PRESS);

    saul_reg_read(devTemp, &temperature);
    saul_reg_read(devHum, &humidity);
    saul_reg_read(devPres, &pressure);

    printf("temperature: ");
    printf("%.1f\n", round(temperature.val[0] * pow(10, temperature.scale) * 10.0) / 10.0);

    printf("humidity: ");
    printf("%d\n", (int) round(humidity.val[0] * pow(10, humidity.scale)));

    printf("pressure: ");
    printf("%d\n", (int) round(pressure.val[0] * pow(10, pressure.scale)));
}

static void* window_sensor_task(void* arg) {

    msg_t message;
    (void) arg;

    msg_init_queue(rcv_queue, RCV_QUEUE_SIZE);

    struct window_sensor* ws;
    struct window_sensor_event event;

    while (true) {

        msg_receive(&message);
        event = *((struct window_sensor_event*) message.content.ptr);
        ws = event.window_sensor;

        bool currentState = event.level;

        unsigned long current_time = (unsigned long) xtimer_now_usec64() / 1000;

        unsigned long time_diff = 0;

        if (current_time < ws->timestamp_last_interrupt) {
            // catch overflow
            time_diff = ws->interrupt_debounce + 1;
        }
        else {
            time_diff = current_time - ws->timestamp_last_interrupt;
        }

        if (time_diff <= (unsigned long) ws->interrupt_debounce) {
            // not within debounce time -> ignore interrupt
            continue;
        }

        char output[128];
        sprintf(output, "window sensor #%i: ", ws->id);
        printf("%s\n", output);

        if (currentState == LOW) {
            printf("open\n");
            ws->last_state = LOW;
            //mqttClient.publish(ws.mqtt_topic, "OPEN", false, 2);
        }
        else if (currentState == HIGH) {
            printf("closed\n");
            ws->last_state = HIGH;
            //mqttClient.publish(ws.mqtt_topic, "CLOSED", false, 2);
        }

        ws->timestamp_last_interrupt = current_time;
    }

    return NULL;
}

void isr_window_sensor_1(void* arg) {

    msg_t message;

    struct window_sensor_event event;
    event.window_sensor = &window_sensor_1;
    event.level = gpio_read(window_sensor_1.gpio_intput);

    message.content.ptr = &event;
    if (msg_try_send(&message, window_sensor_task_pid) == 0) {
        printf("Receiver queue full.\n");
    }
}

void isr_window_sensor_2(void* arg) {

    msg_t message;

    struct window_sensor_event event;
    event.window_sensor = &window_sensor_2;
    event.level = gpio_read(window_sensor_2.gpio_intput);

    message.content.ptr = &event;
    if (msg_try_send(&message, window_sensor_task_pid) == 0) {
        printf("Receiver queue full.\n");
    }
}

void init_window_sensor(struct window_sensor ws) {

    gpio_init_int(ws.gpio_intput, GPIO_IN_PD, GPIO_BOTH, ws.isr, NULL);
    gpio_irq_enable(ws.gpio_intput);

    gpio_init(ws.gpio_output, GPIO_OUT);

    // output always on to detect changes on input
    gpio_write(ws.gpio_output, HIGH);
}

void configureWindowSensorSystem(void) {

    char topic[128];

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

        build_topic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);

        window_sensor_1.id = 1;
        window_sensor_1.gpio_intput = CONFIG_SENSOR_WINDOW_1_GPIO_INPUT;
        window_sensor_1.gpio_output = CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT;
        window_sensor_1.isr = (gpio_cb_t)isr_window_sensor_1;
        window_sensor_1.interrupt_debounce = CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS;
        window_sensor_1.timestamp_last_interrupt = 0;
        window_sensor_1.mqtt_topic = topic;

        init_window_sensor(window_sensor_1);

    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

    build_topic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);

        window_sensor_2.id = 2;
        window_sensor_2.gpio_intput = CONFIG_SENSOR_WINDOW_2_GPIO_INPUT;
        window_sensor_2.gpio_output = CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT;
        window_sensor_2.isr = (gpio_cb_t)isr_window_sensor_2;
        window_sensor_2.interrupt_debounce = CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS;
        window_sensor_2.timestamp_last_interrupt = 0;
        window_sensor_2.mqtt_topic = topic;

        init_window_sensor(window_sensor_2);

    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void initWindowSensorSystem(void) {

    printf("init task queue\n");
    window_sensor_task_pid = thread_create(rcv_stack, sizeof(rcv_stack), THREAD_PRIORITY_MAIN - 1, 0, window_sensor_task, NULL, "window_sensor_task");

    configureWindowSensorSystem();
}

bool setup(void) {

    printf("device is running version: ");
    printf("%s (%i)\n", APP_VERSION_NAME, APP_VERSION_CODE);

    initWindowSensorSystem();

    return true;
}

int main(void) {

    bool success = setup();

    if (!success) {
        return false;
    }

    while (true) {
        publish_environment_data();
        xtimer_usleep(CONFIG_SENSOR_POLL_INTERVAL_MS * 1000);
    }

    return 0;
}
