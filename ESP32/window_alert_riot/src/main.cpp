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

#include "WindowSensor.cpp"

#define LOW 0
#define HIGH 1

#define RCV_QUEUE_SIZE (8)

WindowSensor *pWindowSensor1, *pWindowSensor2;

static kernel_pid_t window_sensor_task_pid;
static char rcv_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
static msg_t rcv_queue[RCV_QUEUE_SIZE];


void build_topic(char *output, const char *room, const char *boardID, const char *measurement) {

    sprintf(output, "room/%s/%s/%s", room, boardID, measurement);
}

void publish_environment_data() {

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

    msg_init_queue(rcv_queue, RCV_QUEUE_SIZE);

    WindowSensor* windowSensor;
    WindowSensorEvent event;

    while (true) {

        msg_receive(&message);
        event = *((WindowSensorEvent*) message.content.ptr);

        windowSensor = event.windowSensor;
        bool currentState = event.level;

        unsigned long current_time = (unsigned long) xtimer_now_usec64() / 1000;

        unsigned long timeDiff = 0;

        if (current_time < windowSensor->getTimestampLastInterrupt()) {
            // catch overflow
            timeDiff = windowSensor->getInterruptDebounce() + 1;
        }
        else {
            timeDiff = current_time - windowSensor->getTimestampLastInterrupt();
        }

        if (timeDiff <= (unsigned long) windowSensor->getInterruptDebounce()) {
            // not within debounce time -> ignore interrupt
            continue;
        }

        char output[128];
        sprintf(output, "window sensor #%i: ", windowSensor->getId());
        printf("%s", output);

        if (currentState == LOW) {
            printf("open");
            windowSensor->setLastState(LOW);
            //mqttClient.publish(windowSensor->getMqttTopic(), "OPEN", false, 2);
        }
        else if (currentState == HIGH) {
            printf("closed");
            windowSensor->setLastState(HIGH);
            //mqttClient.publish(windowSensor->getMqttTopic(), "CLOSED", false, 2);
        }

        windowSensor->setTimestampLastInterrupt(current_time);
    }

    return NULL;
}

void isrWindowSensor1(void* arg) {

    msg_t message;

    WindowSensorEvent event;
    event.windowSensor = pWindowSensor1;
    event.level = gpio_read(pWindowSensor1->getInputGpio());

    message.content.ptr = &event;
    if (msg_try_send(&message, window_sensor_task_pid) == 0) {
        printf("Receiver queue full.\n");
    }
}

void isrWindowSensor2(void* arg) {

    msg_t message;

    WindowSensorEvent event;
    event.windowSensor = pWindowSensor2;
    event.level = gpio_read(pWindowSensor2->getInputGpio());

    message.content.ptr = &event;
    if (msg_try_send(&message, window_sensor_task_pid) == 0) {
        printf("Receiver queue full.\n");
    }
}

void configureWindowSensorSystem() {

    char topic[128];

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

        build_topic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);

        WindowSensor windowSensor1(
                CONFIG_SENSOR_WINDOW_1_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor1 = &windowSensor1;

        pWindowSensor1->initGpio(&isrWindowSensor1);

    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

    build_topic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);

        WindowSensor windowSensor2(
                CONFIG_SENSOR_WINDOW_2_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor2 = &windowSensor2;

        pWindowSensor2->initGpio(&isrWindowSensor2);

    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

void initWindowSensorSystem() {

    printf("init task queue\n");
    window_sensor_task_pid = thread_create(rcv_stack, sizeof(rcv_stack), THREAD_PRIORITY_MAIN - 1, 0, window_sensor_task, NULL, "window_sensor_task");

    configureWindowSensorSystem();
}

bool setup() {

    printf("device is running version: ");
    printf("%s (%i)\n", APP_VERSION_NAME, APP_VERSION_CODE);

    initWindowSensorSystem();

    return true;
}

int main() {

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
