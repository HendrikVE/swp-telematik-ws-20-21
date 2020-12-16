#!/bin/bash

set -e -o xtrace

# load config
source config.sh

echo "Installation of the Mosquitto MQTT Broker is done through the 'openhab-config' tool."
echo "In the following menu please navigate to 'Optional Components -> Mosquitto'"
read -r -p "Press enter to continue..."

sudo openhabian-config

# only continue if Mosquitto is installed
if ! command -v /usr/sbin/mosquitto &> /dev/null
then
    echo "Mosquitto was not found on the system. Abort"
    exit
fi

# copy certificates to mosquitto
pushd "$CA_DIR"
sudo cp ca.crt /etc/mosquitto/ca_certificates/
sudo cp "$HOST_IPV4.crt" "$HOST_IPV4.key" /etc/mosquitto/certs/
popd

# make references in mosquitto config
{
# TODO: solve indentation issue
printf "\n
listener 1883 localhost

listener 8883
cafile /etc/mosquitto/ca_certificates/ca.crt
certfile /etc/mosquitto/certs/%s.crt
keyfile /etc/mosquitto/certs/%s.key
require_certificate true" "$HOST_IPV4" "$HOST_IPV4"
} | sudo tee -a /etc/mosquitto/mosquitto.conf

sudo service mosquitto restart
