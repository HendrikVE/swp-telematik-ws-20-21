#!/bin/bash

set -e -o xtrace

# load config
source config.sh

# initial ssh setup on remote
# assumptions: (1) openHABian running on remote
#              (2) user "openhabian" with password "openhabian"

# use sshpass to easily provide the password for initial ssh access
sudo apt install -y sshpass

# copy public key to remote
read -r -p "Path to your public key: " public_key_file
sshpass -p openhabian scp "$public_key_file" openhabian@openhabiandevice:~
public_key_file_name=$(basename "$public_key_file")

# prepare .ssh and authorized_keys if not already existing
sshpass -p openhabian ssh openhabian@openhabiandevice "mkdir -p .ssh && chmod 700 .ssh"
sshpass -p openhabian ssh openhabian@openhabiandevice "cd .ssh && touch authorized_keys && chmod 600 authorized_keys"

# copy public key to authorized_keys and remove public key file afterwards
sshpass -p openhabian ssh openhabian@openhabiandevice "cat $public_key_file_name >> .ssh/authorized_keys && rm $public_key_file_name"
# from now on the remote device should be accessible by using the correct private key

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
