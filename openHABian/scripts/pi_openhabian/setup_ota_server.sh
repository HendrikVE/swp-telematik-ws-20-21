#!/bin/bash

set -e

# load config
source config.sh

# install nginx
sudo apt install nginx

# copy config file
sudo cp "$RES_DIR/nginx/conf.d/ota.conf" /etc/nginx/conf.d

# replace SERVER_IP in ota.conf with IP address, the certs are already in the resource dir
sudo sed -i -e "s/SERVER_IP/$HOST_IPV4/g" /etc/nginx/conf.d/ota.conf

# restart nginx
sudo service nginx restart
