#ifndef WINDOW_ALERT_WIFI_H
#define WINDOW_ALERT_WIFI_H

#include "freertos/event_groups.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static esp_err_t event_handler(void *ctx, system_event_t *event);

void wifi_init_softap();

void wifi_init_sta();

void init_wifi();

#endif //WINDOW_ALERT_WIFI_H
