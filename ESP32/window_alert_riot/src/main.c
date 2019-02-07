#include "MANIFEST.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "window_sensor.h"

#include "saul_reg.h"
#include "fmt.h"
#include "shell.h"
#include "thread.h"
#include "mutex.h"
#include "xtimer.h"

#include "msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/emcute.h"

extern int _gnrc_netif_config(int argc, char **argv);

#define RCV_QUEUE_SIZE  16

#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 2)
#define WINDOW_SENSOR_PRIO  (THREAD_PRIORITY_MAIN - 1)

static mutex_t emcute_lock;

struct window_sensor window_sensor_1 = {}, window_sensor_2 = {};

static kernel_pid_t window_sensor_task_pid;
static char rcv_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
static msg_t rcv_queue[RCV_QUEUE_SIZE];

static char mqtt_stack[THREAD_STACKSIZE_DEFAULT];

void build_topic(char *output, const char *room, const char *board_id, const char *measurement) {

    sprintf(output, "home/%s/%s/%s", room, board_id, measurement);
}

void publish_mqtt(char *topic_name, char *data) {

    mutex_lock(&emcute_lock);

    emcute_topic_t topic;
    unsigned flags = EMCUTE_QOS_0;
    //flags |= EMCUTE_QOS_2;

    topic.name = topic_name;
    if (emcute_reg(&topic) != EMCUTE_OK) {
        printf("error: unable to obtain topic ID of '%s'\n", topic_name);
    }

    if (emcute_pub(&topic, data, strlen(data), flags) != EMCUTE_OK) {
        printf("error: unable to publish data to topic '%s [%i]'\n", topic.name, (int)topic.id);
    }

    mutex_unlock(&emcute_lock);
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


    char str_temperature[32];
    int len = fmt_float(str_temperature, (float) (round((temperature.val[0] * pow(10, temperature.scale) * 10.0)) / 10.0), 1);
    str_temperature[len] = '\0';
    printf("temperature: %s\n", str_temperature);

    char str_humidity[32];
    sprintf(str_humidity, "%d", (int) round(humidity.val[0] * pow(10, humidity.scale)));
    printf("humidity: %s\n", str_humidity);

    char str_pressure[32];
    sprintf(str_pressure, "%d", (int) round(pressure.val[0] * pow(10, pressure.scale)));
    printf("pressure: %s\n", str_pressure);


    char topic_temperature[128];
    build_topic(topic_temperature, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE);

    char topic_humidity[128];
    build_topic(topic_humidity, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_HUMIDITY);

    char topic_pressure[128];
    build_topic(topic_pressure, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_PRESSURE);

    publish_mqtt(topic_temperature, str_temperature);
    publish_mqtt(topic_humidity, str_humidity);
    publish_mqtt(topic_pressure, str_pressure);

    printf("\n");
}

static void* window_sensor_task(void *arg) {

    msg_t message;

    msg_init_queue(rcv_queue, RCV_QUEUE_SIZE);

    struct window_sensor *window_sensor;
    uint32_t sensor_num;

    bool currentState;

    uint64_t current_time;
    uint64_t time_diff;

    const char* window_state;

    while (true) {

        msg_receive(&message);
        sensor_num = message.content.value;

        if (sensor_num == 1) {
            window_sensor = &window_sensor_1;
            currentState = gpio_read(window_sensor_1.gpio_input);
        }
        else if (sensor_num == 2){
            window_sensor = &window_sensor_2;
            currentState = gpio_read(window_sensor_2.gpio_input);
        }
        else {
            printf("invalid sensor_num: %d", sensor_num);
            continue;
        }

        current_time = xtimer_now_usec64() / 1000;

        time_diff = 0;

        if (current_time < window_sensor->timestamp_last_interrupt) {
            // catch overflow
            time_diff = window_sensor->interrupt_debounce + 1;
        }
        else {
            time_diff = current_time - window_sensor->timestamp_last_interrupt;
        }

        if (time_diff <= (uint64_t) window_sensor->interrupt_debounce) {
            // not within debounce time -> ignore interrupt
            continue;
        }

        printf("window sensor #%i: ", window_sensor->id);

        if (currentState == LOW) {
            printf("open\n");
            window_sensor->last_state = LOW;
            window_state = "ON";
        }
        else if (currentState == HIGH) {
            printf("closed\n");
            window_sensor->last_state = HIGH;
            window_state = "OFF";
        }

        publish_mqtt(window_sensor->mqtt_topic, (char*) window_state);

        window_sensor->timestamp_last_interrupt = current_time;
    }

    return NULL;
}

void message_window_sensor(uint32_t sensor_num) {

    msg_t message;

    message.content.value = sensor_num;
    if (msg_try_send(&message, window_sensor_task_pid) == 0) {
        printf("receiver queue full.\n");
    }
}

void isr_window_sensor_1(void *arg) {
    message_window_sensor(1);
}

void isr_window_sensor_2(void *arg) {
    message_window_sensor(2);
}

void configure_window_sensor_system(void) {

    printf("configure_window_sensor_system\n");

    char topic[128];

    #if CONFIG_SENSOR_WINDOW_1_ENABLED

        build_topic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_1_MQTT_TOPIC);

        window_sensor_1.id = 1;
        window_sensor_1.gpio_input = CONFIG_SENSOR_WINDOW_1_GPIO_INPUT;
        window_sensor_1.gpio_output = CONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT;
        window_sensor_1.interrupt_debounce = CONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS;
        strncpy(window_sensor_1.mqtt_topic, topic, sizeof(window_sensor_1.mqtt_topic));

        window_sensor_init_gpio(&isr_window_sensor_1, &window_sensor_1);

    #endif /*CONFIG_SENSOR_WINDOW_1_ENABLED*/

    #if CONFIG_SENSOR_WINDOW_2_ENABLED

        build_topic(topic, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_WINDOW_2_MQTT_TOPIC);

        window_sensor_2.id = 2;
        window_sensor_2.gpio_input = CONFIG_SENSOR_WINDOW_2_GPIO_INPUT;
        window_sensor_2.gpio_output = CONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT;
        window_sensor_2.interrupt_debounce = CONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS;
        strncpy(window_sensor_2.mqtt_topic, topic, sizeof(window_sensor_2.mqtt_topic));

        window_sensor_init_gpio(&isr_window_sensor_2, &window_sensor_2);

    #endif /*CONFIG_SENSOR_WINDOW_2_ENABLED*/
}

static void* emcute_thread(void* arg) {

    emcute_run(CONFIG_MQTTSN_GATEWAY_PORT, CONFIG_DEVICE_ID);

    return NULL;
}

void init_window_sensor_system(void) {

    configure_window_sensor_system();

    printf("init task queue for window sensors\n");
    window_sensor_task_pid = thread_create(rcv_stack, sizeof(rcv_stack), WINDOW_SENSOR_PRIO, 0, window_sensor_task, NULL, "window_sensor_task");
}

void init_mqtt(void) {

    printf("init task queue for mqtt\n");
    thread_create(mqtt_stack, sizeof(mqtt_stack), EMCUTE_PRIO, 0, emcute_thread, NULL, "emcute_task");

    sock_udp_ep_t gw = { .family = AF_INET6, .port = CONFIG_MQTTSN_GATEWAY_PORT };

    ipv6_addr_from_str((ipv6_addr_t*) &gw.addr.ipv6, CONFIG_MQTTSN_GATEWAY_IP);

    if (emcute_con(&gw, true, NULL, NULL, 0, 0) != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", CONFIG_MQTTSN_GATEWAY_IP, (int)gw.port);
        return false;
    }
    printf("Successfully connected to gateway at [%s]:%i\n", CONFIG_MQTTSN_GATEWAY_IP, (int)gw.port);
}

bool setup(void) {

    /* add global routed IPv6 */
    char **cmd = (char*[]) {"ifconfig", "7", "add", "affe::1"};
    _gnrc_netif_config(4, cmd);

    mutex_init(&emcute_lock);

    init_mqtt();

    char str_version[16];
    sprintf(str_version, "%s (%i)", APP_VERSION_NAME, APP_VERSION_CODE);

    printf("device is running version: %s\n", str_version);

    char topic_version[128];
    build_topic(topic_version, CONFIG_DEVICE_ROOM, CONFIG_DEVICE_ID, CONFIG_SENSOR_MQTT_TOPIC_VERSION);

    publish_mqtt(topic_version, str_version);

    init_window_sensor_system();

    return true;
}

int main(void) {

    bool success = setup();

    if (!success) {
        return false;
    }

    message_window_sensor(1);
    message_window_sensor(2);

    while (true) {
        publish_environment_data();
        xtimer_usleep(CONFIG_SENSOR_POLL_INTERVAL_MS * 1000);
    }

    return 0;
}
