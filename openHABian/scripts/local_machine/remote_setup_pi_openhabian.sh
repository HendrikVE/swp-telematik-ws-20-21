#!/bin/bash

set -e -o xtrace

# load config
source config.sh

# check for existing config of host openhabian_pi
if ! grep -Fxq "Host openhabian_pi" "$HOME/.ssh/config"
then
    # get missing config information from user
    printf "\nadd host entry for openhabian to local ssh config\n"
    read -r -p "IP of openhabian: " ssh_config_ip
    read -r -p "Path to private key: " ssh_config_keyfile

    # write local config
    {
        printf "\n"
        echo "Host openhabian_pi"
        echo "        Hostname $ssh_config_ip"
        echo "        User openhabian"
        echo "        IdentityFile $ssh_config_keyfile"
        printf "\n"
    } >> ~/.ssh/config
fi

# TODO: copy public key to remote

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
