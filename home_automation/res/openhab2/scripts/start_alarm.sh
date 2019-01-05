#!/usr/bin/env bash
set -e

if pgrep -x "mpg123" > /dev/null && [ -f /var/lib/openhab2/.mpg123_windowalert_pid ]
then
    echo "alarm is already running!"
    exit 1
fi

mpg123 --loop -1 -q /etc/openhab2/sounds/alarm.mp3 </dev/null >/dev/null 2>&1 &
echo $! | tee /var/lib/openhab2/.mpg123_windowalert_pid
