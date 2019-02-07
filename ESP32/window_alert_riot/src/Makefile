CFLAGS += -include $(APPDIR)/overrides/bmx280_params.h
CFLAGS += -include $(APPDIR)/overrides/periph_conf.h

# name of your application
APPLICATION = window_alert

# If no BOARD is found in the environment, use this default:
BOARD ?= esp32-wroom-32

BOARD_WHITELIST := esp32-wroom-32

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

USEMODULE += esp_wifi

USEMODULE += saul_default
USEMODULE += bme280

USEMODULE += xtimer
USEMODULE += periph_gpio_irq

USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_ipv6_default
USEMODULE += emcute

USEMODULE += shell
USEMODULE += shell_commands

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

CFLAGS += -DCONFIG_SENSOR_WINDOW_1_ENABLED=1
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_ENABLED=1

CFLAGS += -DESP_WIFI_SSID=\"thecakeisalie\"
CFLAGS += -DESP_WIFI_PASS=\"knowyourparadoxes\"

CFLAGS += -DCONFIG_DEVICE_ROOM=\"workroom\"
CFLAGS += -DCONFIG_DEVICE_ID=\"esp32-riot-1\"

CFLAGS += -DCONFIG_SENSOR_POLL_INTERVAL_MS=10000

CFLAGS += -DCONFIG_SENSOR_WINDOW_1_GPIO_INPUT=14
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT=15
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS=100
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_MQTT_TOPIC=\"window_1\"

CFLAGS += -DCONFIG_SENSOR_WINDOW_2_GPIO_INPUT=25
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT=26
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS=100
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_MQTT_TOPIC=\"window_2\"

CFLAGS += -DCONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE=\"temperature\"
CFLAGS += -DCONFIG_SENSOR_MQTT_TOPIC_HUMIDITY=\"humidity\"
CFLAGS += -DCONFIG_SENSOR_MQTT_TOPIC_PRESSURE=\"pressure\"
CFLAGS += -DCONFIG_SENSOR_MQTT_TOPIC_VERSION=\"version\"

CFLAGS += -DCONFIG_MQTT_USERNAME=\"homeassistant\"
CFLAGS += -DCONFIG_MQTT_PASSWORD=\"aperturescience\"
CFLAGS += -DCONFIG_MQTTSN_GATEWAY_IP=\"ff02::1\"
CFLAGS += -DCONFIG_MQTTSN_GATEWAY_PORT=1885

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
