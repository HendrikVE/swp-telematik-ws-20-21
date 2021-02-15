#!/bin/bash

set -e

# load config
source config.sh

BACKUP_DIR=/var/lib/openhab/jsondb/manual_backup

# make backups of existing config files
for file in "$RES_DIR"/openhab3/var/lib/openhab/jsondb/*
do
  BASE_FILE=$(basename "$file")
  TIMESTAMP=$(date +%s)
  sudo mkdir -p "$BACKUP_DIR"
  sudo mv "$file" "$BACKUP_DIR/$TIMESTAMP--$BASE_FILE"
done

# copy files
sudo cp -r "$RES_DIR/openhab3/etc/openhab/sounds" /etc/openhab/sounds
sudo cp -r "$RES_DIR/openhab3/var/lib/openhab/jsondb" /var/lib/openhab/jsondb
