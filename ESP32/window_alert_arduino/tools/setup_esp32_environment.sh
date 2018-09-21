#!/bin/bash
# script was designed for Ubuntu
# based on https://esp-idf.readthedocs.io/en/v2.0/linux-setup.html

set -e

printf "\nget necessary software\n"
sudo apt-get install git wget make libncurses-dev flex bison gperf python python-serial

printf "\nsetup working directory\n"
dir_name=esp32
wd=~/$dir_name

mkdir $wd

cd $wd

printf "\ndownload correct file corresponding to system architecture\n"
arch=$(uname -m)

if [ $arch == "x86_64" ]
then
    file=xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

elif [ $arch == "i686" ] || [ $arch == "i386" ]
then
    file=xtensa-esp32-elf-linux32-1.22.0-80-g6c4433a-5.2.0.tar.gz

else
    >&2 printf "system not supported\n"
    exit 1
fi

wget "https://dl.espressif.com/dl/$file"

tar -xzf $file && rm $file

printf "\nsetup esp-idf\n"
git clone https://github.com/espressif/esp-idf.git
git -C esp-idf checkout v3.1
git -C esp-idf submodule update --init --recursive

printf "\nupdate ~/.profile\n"
echo "# ESP32" >> ~/.profile
echo "export PATH=\"\$PATH:\$HOME/$dir_name/xtensa-esp32-elf/bin\"" >> ~/.profile
echo "export IDF_PATH=\"\$HOME/$dir_name/esp-idf\"" >> ~/.profile

source ~/.profile
