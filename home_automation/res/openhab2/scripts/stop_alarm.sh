#!/usr/bin/env bash
set -e

pid=$(cat /var/lib/openhab2/.mpg123_windowalert_pid)
rm /var/lib/openhab2/.mpg123_windowalert_pid
kill $pid
