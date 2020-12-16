#!/bin/bash

set -e -o xtrace

# load config
source config.sh

# for reference see: http://rockingdlabs.dunmire.org/exercises-experiments/ssl-client-certs-to-secure-mqtt
# certificates generated as shown here: https://jamielinux.com/docs/openssl-certificate-authority/index.html

client_name_1="esp32-1"
client_name_2="esp32-2"

res_dir="../res/CA"

ca_dir="$HOME/CA_test"
mkdir -p "$ca_dir"
chmod 700 "$ca_dir"

cp "$res_dir/generate-CA.sh" "$res_dir/setup-CA.sh" "$ca_dir"
cd "$ca_dir"

# create ca cert and server cert + key
IPLIST="$HOST_IPV4" HOSTLIST="$HOST_IPV4" ./generate-CA.sh

# script is naming certs based on hostname, rename them
mv "$HOST_NAME.crt" "$HOST_IPV4.crt"
mv "$HOST_NAME.csr" "$HOST_IPV4.csr"
mv "$HOST_NAME.key" "$HOST_IPV4.key"

# create client cert + key for esp32-1
IPLIST="$HOST_IPV4" HOSTLIST="$HOST_IPV4" ./generate-CA.sh client "$client_name_1"

# create client cert + key for esp32-2
IPLIST="$HOST_IPV4" HOSTLIST="$HOST_IPV4" ./generate-CA.sh client "$client_name_2"
