#!/bin/bash
set -e

pid=$(cat /var/lib/openhab/.mpg123_windowalert_pid)
rm /var/lib/openhab/.mpg123_windowalert_pid
kill $pid
