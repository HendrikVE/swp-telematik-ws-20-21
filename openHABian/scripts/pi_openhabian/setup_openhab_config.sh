#!/bin/bash

set -e

# load config
source config.sh

sudo systemctl stop openhab

JSONDB_DIR=/var/lib/openhab/jsondb
BACKUP_DIR="$JSONDB_DIR/manual_backup"

# make backups of existing config files
set +e
for file in "$RES_DIR"/openhab3/var/lib/openhab/jsondb/*
do
  BASE_FILE=$(basename "$file")
  TIMESTAMP=$(date +%s)
  sudo mkdir -p "$BACKUP_DIR"
  sudo mv "$JSONDB_DIR/$BASE_FILE" "$BACKUP_DIR/$TIMESTAMP--$BASE_FILE"
done
set -e

# install requirements for our openHAB setup
sudo apt install -y mpg321

# copy files
sudo -u openhab cp -r "$RES_DIR/openhab3/etc/openhab/scripts/." /etc/openhab/scripts
sudo chmod +x /etc/openhab/scripts/start_alarm.sh
sudo chmod +x /etc/openhab/scripts/stop_alarm.sh

sudo -u openhab cp -r "$RES_DIR/openhab3/etc/openhab/sounds/." /etc/openhab/sounds
sudo -u openhab cp -r "$RES_DIR/openhab3/var/lib/openhab/jsondb/." /var/lib/openhab/jsondb

sudo systemctl start openhab
