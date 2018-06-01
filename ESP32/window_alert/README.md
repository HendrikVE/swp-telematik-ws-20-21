# SETUP

0. Setup build environment. If you are using Ubuntu you can run **./setup_esp32_environment.sh**
1. run **make menuconfig** and exchange placeholder (e.g. network name and password) with your data

# PROBLEMS
- Workaround for MQTT: https://github.com/tuanpmt/espmqtt/issues/48

# Config
Within **make menuconfig** do the following (optional) configurations:
1. To save energy
    - enable **Component config -> FreeRTOS -> Run FreeRTOS only on first core**
2. To fix problem with MQTT (https://github.com/tuanpmt/espmqtt/issues/48)
    - enable **Component config -> LWIP -> Support LWIP socket select() only**
