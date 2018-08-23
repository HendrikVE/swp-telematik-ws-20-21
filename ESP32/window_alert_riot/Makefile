# name of your application
APPLICATION = window_alert

# If no BOARD is found in the environment, use this default:
BOARD ?= esp32-generic

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../..

USEMODULE += esp_wifi

USEMODULE += saul_default
USEMODULE += bme280

USEMODULE += xtimer

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
# CFLAGS += -DDEVELHELP
CFLAGS += -DBMX280_PARAM_I2C_ADDR=0x76

# CFLAGS += -DI2C0_SCL_ADDR=GPIO27
# CFLAGS += -DI2C0_SDA=GPIO23

CFLAGS += -DCONFIG_SENSOR_WINDOW_1_ENABLED=1
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_ENABLED=1

CFLAGS += -DCONFIG_WIFI_STA_SSID=\"SSID\"
CFLAGS += -DCONFIG_WIFI_STA_PASSWORD=\"PASSWORD\"

CFLAGS += -DCONFIG_DEVICE_ROOM=\"livingroom\"
CFLAGS += -DCONFIG_DEVICE_ID=\"esp32-riot\"

CFLAGS += -DCONFIG_SENSOR_POLL_INTERVAL_MS=60000

CFLAGS += -DCONFIG_SENSOR_WINDOW_1_GPIO_INPUT=14
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_GPIO_OUTPUT=15
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_INTERRUPT_DEBOUNCE_MS=100
CFLAGS += -DCONFIG_SENSOR_WINDOW_1_MQTT_TOPIC=\"WindowSensor1\"

CFLAGS += -DCONFIG_SENSOR_WINDOW_2_GPIO_INPUT=25
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_GPIO_OUTPUT=26
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_INTERRUPT_DEBOUNCE_MS=100
CFLAGS += -DCONFIG_SENSOR_WINDOW_2_MQTT_TOPIC=\"WindowSensor2\"

FEATURES_REQUIRED += cpp

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
