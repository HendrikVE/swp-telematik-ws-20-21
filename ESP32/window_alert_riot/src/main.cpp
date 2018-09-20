#include "MANIFEST.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "saul_reg.h"
#include "xtimer.h"
#include "periph/gpio.h"

#include "thread.h"
/*
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"
*/

#include "WindowSensor.cpp"
#include "ConnectivityManager.cpp"
#include "FlashStorage.h"

#define LOW 0
#define HIGH 1

#define RCV_QUEUE_SIZE  8
#define MQTT_QUEUE_SIZE 8

#define EMCUTE_PRIO (THREAD_PRIORITY_MAIN - 1)

#define NUMOFSUBS       16U
#define TOPIC_MAXLEN    64U

WindowSensor *pWindowSensor1, *pWindowSensor2;

ConnectivityManager connectivityManager;

static kernel_pid_t windowSensorTaskPid;
static char rcvStack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
static msg_t rcvQueue[RCV_QUEUE_SIZE];

static char mqttStack[THREAD_STACKSIZE_DEFAULT];
//static msg_t mqttQueue[MQTT_QUEUE_SIZE];


void buildTopic(char *output, const char *room, const char *boardID, const char *measurement) {

    sprintf(output, "room/%s/%s/%s", room, boardID, measurement);
}

void publishMqtt(char* topicName, char* data) {

    /*emcute_topic_t topic;
    unsigned flags = EMCUTE_QOS_0;
    flags |= EMCUTE_QOS_2;

    topic.name = topicName;
    if (emcute_reg(&topic) != EMCUTE_OK) {
        puts("error: unable to obtain topic ID");
    }

    if (emcute_pub(&topic, data, strlen(data), flags) != EMCUTE_OK) {
        printf("error: unable to publish data to topic '%s [%i]'\n", topic.name, (int)topic.id);
    }*/
}

void publishEnvironmentData() {

    phydat_t temperature;
    phydat_t humidity;
    phydat_t pressure;

    saul_reg_t* devTemp = saul_reg_find_type(SAUL_SENSE_TEMP);
    saul_reg_t* devHum = saul_reg_find_type(SAUL_SENSE_HUM);
    saul_reg_t* devPres = saul_reg_find_type(SAUL_SENSE_PRESS);

    saul_reg_read(devTemp, &temperature);
    saul_reg_read(devHum, &humidity);
    saul_reg_read(devPres, &pressure);


    char strTemperature[32];
    sprintf(strTemperature, "%.1f", round(temperature.val[0] * pow(10, temperature.scale) * 10.0) / 10.0);

    printf("temperature: %s\n", strTemperature);

    char strHumidity[32];
    sprintf(strHumidity, "%d", (int) round(humidity.val[0] * pow(10, humidity.scale)));

    printf("humidity: %s\n", strHumidity);

    char strPressure[32];
    sprintf(strPressure, "%d", (int) round(pressure.val[0] * pow(10, pressure.scale)));

    printf("pressure: %s\n", strPressure);


    char topicTemperature[128];
    buildTopic(topicTemperature, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE);

    char topicHumidity[128];
    buildTopic(topicHumidity, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_HUMIDITY);

    char topicPressure[128];
    buildTopic(topicPressure, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_PRESSURE);

    publishMqtt(topicTemperature, strTemperature);
    publishMqtt(topicHumidity, strHumidity);
    publishMqtt(topicPressure, strPressure);


    printf("\n");
}

static void* windowSensorTask(void* arg) {

    msg_t message;

    msg_init_queue(rcvQueue, RCV_QUEUE_SIZE);

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

        const char* windowState;

        if (currentState == LOW) {
            printf("open");
            windowSensor->setLastState(LOW);
            windowState = "OPEN";
        }
        else if (currentState == HIGH) {
            printf("closed");
            windowSensor->setLastState(HIGH);
            windowState = "CLOSED";
        }
        printf("\n");

        publishMqtt(windowSensor->getMqttTopic(), (char*) windowState);

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
    if (msg_try_send(&message, windowSensorTaskPid) == 0) {
        printf("Receiver queue full.\n");
    }
}

void isrWindowSensor2(void* arg) {

    msg_t message;

    WindowSensorEvent event;
    event.windowSensor = pWindowSensor2;
    event.level = gpio_read(pWindowSensor2->getInputGpio());

    message.content.ptr = &event;
    if (msg_try_send(&message, windowSensorTaskPid) == 0) {
        printf("Receiver queue full.\n");
    }
}

void configureWindowSensorSystem() {

    char topic[128];

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

    buildTopic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);

        WindowSensor windowSensor1(
                CONFIG_SENSOR_WINDOW_1_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor1 = &windowSensor1;

        pWindowSensor1->initGpio(&isrWindowSensor1);

    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

    buildTopic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);

        WindowSensor windowSensor2(
                CONFIG_SENSOR_WINDOW_2_GPIO_INPUT,
                CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT,
                CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS,
                topic);

        pWindowSensor2 = &windowSensor2;

        pWindowSensor2->initGpio(&isrWindowSensor2);

    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

static void* emcuteThread(void* arg) {

    //emcute_run(CONFIG_MQTTSN_GATEWAY_PORT, CONFIG_DEVICE_ID);

    return NULL;
}

void initWindowSensorSystem() {

    printf("init task queue\n");
    windowSensorTaskPid = thread_create(rcvStack, sizeof(rcvStack), THREAD_PRIORITY_MAIN - 1, 0, windowSensorTask, NULL, "windowSensorTask");

    configureWindowSensorSystem();

    thread_create(mqttStack, sizeof(mqttStack), EMCUTE_PRIO, 0, emcuteThread, NULL, "emcute");
}

bool setup() {

    printf("device is running version: ");
    printf("%s (%i)\n", APP_VERSION_NAME, APP_VERSION_CODE);

    initWindowSensorSystem();

    connectivityManager.begin();
    connectivityManager.initMqtt();

    return true;
}

int main() {

    bool success = setup();

    if (!success) {
        return false;
    }

    while (true) {
        publishEnvironmentData();
        xtimer_usleep(CONFIG_SENSOR_POLL_INTERVAL_MS * 1000);
    }

    return 0;
}
