#!/bin/bash

printf "\nsetup SSH\n"
# source setup_ssh.sh

printf "\nsetup CA\n"
source setup_ca.sh

printf "\nsetup openhab2 project config\n"
# source setup_openhab2_config.sh

printf "\nsetup mosquitto\n"
source setup_mosquitto.sh

printf "\nsetup OTA server\n"
# source setup_ota_server.sh

printf "\nsetup lets encrypt\n"
# source setup_lets_encrypt.sh
