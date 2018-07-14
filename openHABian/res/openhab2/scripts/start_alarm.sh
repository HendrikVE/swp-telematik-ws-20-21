#!/usr/bin/env bash
set -e

if pgrep -x "mplayer" > /dev/null && [ -f /var/lib/openhab2/.mplayer_pid ]
then
    echo "alarm is already running!"
    exit 1
fi

mplayer -nogui /etc/openhab2/sounds/alarm.mp3 -loop 0 -volume 100 -really-quiet </dev/null >/dev/null 2>&1 &
echo $! | tee /var/lib/openhab2/.mplayer_pid
