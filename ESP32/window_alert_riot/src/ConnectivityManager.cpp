#include <stdbool.h>

#include "mutex.h"
#include "msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/emcute.h"


class ConnectivityManager {

public:

    ConnectivityManager() {
        mutex_init(&mMqttMutex);
    }

    void begin() {

    }

    bool checkMqttConnection() {

        mutex_lock(&mMqttMutex);

        sock_udp_ep_t gw;
        gw.family = AF_INET;
        gw.port = CONFIG_MQTT_BROKER_PORT;

        ipv6_addr_from_str((ipv6_addr_t*) &gw.addr.ipv6, CONFIG_MQTT_BROKER_IP);

        char *topic = NULL;
        char *message = NULL;
        size_t len = 0;

        if (emcute_con(&gw, true, topic, message, len, 0) != EMCUTE_OK) {
            printf("error: unable to connect to [%s]:%i\n", CONFIG_MQTT_BROKER_IP, (int)gw.port);
            return false;
        }
        printf("Successfully connected to gateway at [%s]:%i\n", CONFIG_MQTT_BROKER_IP, (int)gw.port);

        mutex_unlock(&mMqttMutex);

        return true;
    }

    bool initMqtt() {

        return checkMqttConnection();
    }

private:

    mutex_t mMqttMutex;

};
