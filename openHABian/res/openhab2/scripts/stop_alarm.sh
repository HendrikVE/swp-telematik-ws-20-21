#!/usr/bin/env bash
set -e

pid=$(cat /var/lib/openhab2/.mplayer_pid)
rm /var/lib/openhab2/.mplayer_pid
kill $pid
