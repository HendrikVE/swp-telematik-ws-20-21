#!/bin/bash

set -e

flash() {
    printf "room: "
    read inputRoom

    printf "device ID: "
    read inputID

    deviceRoom='"'$inputRoom'"'
    deviceID='"'$inputID'"'

    sed -i "/CONFIG_DEVICE_ROOM=/c\CONFIG_DEVICE_ROOM=$deviceRoom" ../sdkconfig
    sed -i "/CONFIG_DEVICE_ID=/c\CONFIG_DEVICE_ID=$deviceID" ../sdkconfig

    dir=$(pwd)
    cd ..
    make flash monitor
    cd $dir
}

printf "number of flash processes: "
read inputCount

if ! [[ "$inputCount" =~ ^[0-9]+$ ]]; then
    echo "FAILED: input must be a number!"
    exit 1
fi

counter=0
while (($counter < $inputCount )); do
    let counter=counter+1
    flash
done
