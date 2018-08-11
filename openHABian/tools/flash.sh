#!/bin/bash

set -e

if [ $# -ne 2 ]; then
    >&2 echo "script needs 2 arguments (room, ID)!"
    exit 1
fi

deviceRoom='"'$1'"'
deviceID='"'$2'"'

sed -i "/CONFIG_DEVICE_ROOM=/c\CONFIG_DEVICE_ROOM=$deviceRoom" ../sdkconfig
sed -i "/CONFIG_DEVICE_ID=/c\CONFIG_DEVICE_ID=$deviceID" ../sdkconfig

dir=$(pwd)
cd ..
make flash
cd $dir
