# SETUP

0. Clone this repository with **"git clone --recurse-submodules https://github.com/HendrikVE/smarthome2"**
1. Setup build environment. If you are using Ubuntu you can run **./setup_esp32_environment.sh**
2. run **make menuconfig**
    1. exchange placeholder (e.g. network name and password) with your data
    2. Serial flasher config -> Flash size (adjust value to your board)
    3. Compiler options -> Enable C++ exceptions (make a tick here)
    4. Arduino Configuration -> Autostart Arduino setup and loop on boot (make a tick here)
    5. Arduino Configuration -> Used partition scheme -> Minimal SPIFFS (for large apps with OTA) (make a tick here)
    6. Partition Table -> Partition Table -> Minimal SPIFFS (for large apps with OTA)
    7. Component config -> ESP32-specific -> CPU frequency (set it to 240 MHz for low latency of the system)
3. Hardware
    1. Dont use the 5V pin when using battery, this pin is only powered when connected to USB

# Config
Within **make menuconfig** do the following (optional) configurations:
1. To save energy
    - enable **Component config -> FreeRTOS -> Run FreeRTOS only on first core**
    - DON'T set **Component config -> ESP32-specific -> CPU frequency** to 80 MHz
2. To fix problem with MQTT (https://github.com/tuanpmt/espmqtt/issues/48)
    - enable **Component config -> LWIP -> Support LWIP socket select() only**
3. To make binary smaller
    - Arduino Configuration -> Include only specific Arduino libraries (make a tick here)
        - only include the following:
            - BLE
            - HTTPClient
            - SPI
            - Update
            - WiFi
            - WiFiClientSecure
            - Wire
            - Adafruit_BME280
            - Adafruit_BME680
            - Adafruit_Sensor
            - Arduino_Log
            - arduino_mqtt

# Build for production use
1. Log output -> Default log verbosity -> No output (make a tick here)
2. Arduino Configuration -> Debug Log Configuration -> Default log level -> No output (make a tick here)
3. Build -> Build variant -> Production (make a tick here)
4. Compiler options -> Optimization Level -> Release (-Os) (make a tick here)
