#!/bin/bash

HOSTNAME=$(hostname -I | cut -d " " -f 1)

# create server certificate
IPLIST="$HOSTNAME" HOSTLIST="$HOSTNAME" ./generate-CA.sh

# create client certificate
IPLIST="$HOSTNAME" HOSTLIST="$HOSTNAME" ./generate-CA.sh client "ESP32"
