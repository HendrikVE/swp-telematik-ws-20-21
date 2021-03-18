# SETUP

1. Setup build environment.  Run **./tools/setup_esp32_environment.sh** and then **./tools/init_project.sh**
2. Refer to the wiki **Getting Started** for next steps of setup, below are optional configurations

# Technical stuff

These should already be defaults.

1. run **make menuconfig**
    1. Serial flasher config -> Flash size (adjust value to your board)
    2. Serial flasher config -> Default baud rate -> 921600 baud (for faster flashing)
    3. Compiler options -> Enable C++ exceptions (make a tick here)
    4. Arduino Configuration -> Autostart Arduino setup and loop on boot (make a tick here)
    5. Partition Table -> Partition Table -> Factory app, two OTA definitions
    6. Component config -> ESP32-specific -> CPU frequency (set it to 240 MHz for low latency of the system)
2. Hardware
    1. Dont use the 5V pin when using battery, this pin is only powered when connected to USB

# Optional Configurations
Within **make menuconfig** do the following (optional) configurations:
1. To save energy
    - enable **Component config -> FreeRTOS -> Run FreeRTOS only on first core**
    - DON'T set **Component config -> ESP32-specific -> CPU frequency** to 80 MHz
2. To fix problem with MQTT (https://github.com/tuanpmt/espmqtt/issues/48)
    - enable **Component config -> LWIP -> Support LWIP socket select() only**
3. To make binary smaller
    - Arduino Configuration -> Include only specific Arduino libraries (make a tick here)
        - only include the following:
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
