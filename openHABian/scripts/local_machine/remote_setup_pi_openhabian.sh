#!/bin/bash

set -e

# load config
source config.sh

# initial ssh setup on remote
# assumptions: (1) openHABian running on remote
#              (2) user "openhabian" with password "openhabian"

# check for existing config of host openhabian_pi
if ! grep -Fxq "Host openhabian_pi" "$HOME/.ssh/config"
then
    # get missing config information from user
    printf "\nadd host entry for openhabian to local ssh config\n"
    read -r -p "Path to private key: " ssh_config_keyfile

    # write local config
    {
        printf "\n"
        echo "Host openhabian_pi"
        echo "        Hostname openhabiandevice"
        echo "        User openhabian"
        echo "        IdentityFile $ssh_config_keyfile"
        printf "\n"
    } >> ~/.ssh/config
fi

# create directories on remote side
ssh openhabian_pi "mkdir -p $REMOTE_RES_DIR"
ssh openhabian_pi "mkdir -p $REMOTE_SCRIPT_DIR"

# copy resources for setup scripts
scp -r "$LOCAL_RES_DIR"/* openhabian_pi:"$REMOTE_RES_DIR"

# copy setup scripts
scp -r "$LOCAL_SCRIPT_DIR"/* openhabian_pi:"$REMOTE_SCRIPT_DIR"

# execute setup on remote side
# -t flag is needed for visual menus and sudo password prompts
ssh -t openhabian_pi "cd $REMOTE_SCRIPT_DIR && ./setup_all.sh"

# download generated certificates
./get_certs_from_remote.sh
