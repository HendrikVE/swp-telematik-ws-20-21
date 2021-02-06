#!/bin/bash
# script was designed for Ubuntu 20.04
# based on https://docs.espressif.com/projects/esp-idf/en/v3.3.3/get-started/linux-setup.html

set -e

printf "\nget necessary software\n"
sudo apt update
sudo apt-get install gcc git wget make libncurses-dev flex bison gperf libffi-dev libssl-dev \
     python python3-pip python-setuptools python3-serial python-cryptography python3-future python-pyparsing

printf "\nsetup working directory\n"
toolchain_dir_name=esp32
wd=~/$toolchain_dir_name

mkdir $wd

cd $wd

printf "\ndownload toolchain\n"
xtensa_file="xtensa-esp32-elf-linux64-1.22.0-96-g2852398-5.2.0.tar.gz"
wget "https://dl.espressif.com/dl/$xtensa_file"

tar -xzf "$xtensa_file" && rm "$xtensa_file"

printf "\nsetup esp-idf\n"
git clone https://github.com/espressif/esp-idf.git
git -C esp-idf checkout d3e562907 # dependency given by used arduino-esp32 repository
git -C esp-idf submodule update --init --recursive

printf "\nupdate ~/.bashrc\n"
file_bashrc="$HOME/.bashrc"
{
  printf "\n# ESP32"
  echo "export PATH=\"\$PATH:\$HOME/$toolchain_dir_name/xtensa-esp32-elf/bin\""
  echo "export IDF_PATH=\"\$HOME/$toolchain_dir_name/esp-idf\""
  echo "export PATH=\"\$IDF_PATH/tools:\$PATH\""
} >> "$file_bashrc"

# shellcheck disable=SC1090
source "$file_bashrc"
