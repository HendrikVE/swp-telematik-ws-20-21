# name of your application
APPLICATION = window_alert

# If no BOARD is found in the environment, use this default:
BOARD ?= esp32-generic

BOARD_WHITELIST := esp32-generic

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../../..

USEMODULE += esp_wifi

USEMODULE += saul_default
USEMODULE += bme280

USEMODULE += xtimer

USEMODULE += gnrc_netdev_default
# USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_ipv6_default
USEMODULE += emcute

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

CFLAGS += -DBMX280_PARAM_I2C_ADDR=0x76

# CFLAGS += -DI2C0_SCL_ADDR=GPIO27
# CFLAGS += -DI2C0_SDA=GPIO23

CFLAGS += -DCONFIG_SENSOR_WINDOW_1_ENABLED=1
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_ENABLED=1

CFLAGS += -DCONFIG_WIFI_STA_SSID=\"SSID\"
CFLAGS += -DCONFIG_WIFI_STA_PASSWORD=\"PASSWORD\"

CFLAGS += -DCONFIG_DEVICE_ROOM=\"livingroom\"
CFLAGS += -DCONFIG_DEVICE_ID=\"esp32-riot\"

CFLAGS += -DCONFIG_SENSOR_POLL_INTERVAL_MS=2000

CFLAGS += -DCONFIG_SENSOR_WINDOW_1_GPIO_INPUT=14
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT=15
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS=100
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_MQTT_TOPIC=\"WindowSensor1\"

CFLAGS += -DCONFIG_SENSOR_WINDOW_2_GPIO_INPUT=25
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT=26
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS=100
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_MQTT_TOPIC=\"WindowSensor2\"

CFLAGS += -DCONFIG_SENSOR_MQTT_TOPIC_TEMPERATURE=\"temperature\"
CFLAGS += -DCONFIG_SENSOR_MQTT_TOPIC_HUMIDITY=\"humidity\"
CFLAGS += -DCONFIG_SENSOR_MQTT_TOPIC_PRESSURE=\"pressure\"

# CFLAGS += -DCONFIG_MQTT_USER=\"openhabian\"
# CFLAGS += -DCONFIG_MQTT_PASSWORD=\"password\"
CFLAGS += -DCONFIG_MQTT_SERVER_IP=\"192.168.2.110\"
CFLAGS += -DCONFIG_MQTT_SERVER_PORT=18883

FEATURES_REQUIRED += cpp

CXXEXFLAGS += -std=c++11

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
