#!/bin/bash

# TODO: passwd openhabian

printf "\nsetup CA\n"
source setup_ca.sh

printf "\nsetup openhab project config\n"
source setup_openhab_config.sh

printf "\nsetup mosquitto\n"
source setup_mosquitto.sh

printf "\nsetup OTA server\n"
source setup_ota_server.sh
