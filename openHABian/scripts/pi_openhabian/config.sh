#!/bin/bash

HOST_IPV4=$(hostname -I | cut -d " " -f 1)
HOST_NAME=$(hostname)
export HOST_IPV4 HOST_NAME

export RES_DIR="$HOME/setup/res"

export CA_DIR="$HOME/CA"
