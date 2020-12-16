#!/bin/bash

set -e -o xtrace

# load config
source config.sh

cert_dir="certs_for_window_alert"
mkdir -p "$cert_dir"

# get CA certificate of raspberry pi with openhabian
scp "openhabian_pi:$REMOTE_CA_DIR/ca.crt" "$cert_dir/ca.crt"

# get certificates for first esp32 (esp32-1)
scp "openhabian_pi:$REMOTE_CA_DIR/esp32-1.crt" "$cert_dir/esp32-1.crt"
scp "openhabian_pi:$REMOTE_CA_DIR/esp32-1.key" "$cert_dir/esp32-1.key"

# get certificates for second esp32 (esp32-2)
scp "openhabian_pi:$REMOTE_CA_DIR/esp32-2.crt" "$cert_dir/esp32-2.crt"
scp "openhabian_pi:$REMOTE_CA_DIR/esp32-2.key" "$cert_dir/esp32-2.key"

echo "Finished downloading certificates to 'certs_for_window_alert'. Copy them as needed to 'ESP32/window_alert/src/storage/certs'"
