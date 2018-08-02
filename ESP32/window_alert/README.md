# SETUP

0. Clone this repository with **"git clone --recurse-submodules https://github.com/fu-ilab-swp18/smarthome2"**
1. Setup build environment. If you are using Ubuntu you can run **./setup_esp32_environment.sh**
2. run **make menuconfig**
    1. exchange placeholder (e.g. network name and password) with your data
    2. Serial flasher config -> Flash size (adjust value to your board)
    3. Compiler options -> Enable C++ exceptions (make a tick here)
    4. Arduino Configuration -> Autostart Arduino setup and loop on boot (make a tick here)
    5. Partition Table -> Partition Table -> Factory app, two OTA definitions

# Config
Within **make menuconfig** do the following (optional) configurations:
1. To save energy
    - enable **Component config -> FreeRTOS -> Run FreeRTOS only on first core**
    - set **Component config -> ESP32-specific -> CPU frequency** to 80 MHz
2. To fix problem with MQTT (https://github.com/tuanpmt/espmqtt/issues/48)
    - enable **Component config -> LWIP -> Support LWIP socket select() only**
