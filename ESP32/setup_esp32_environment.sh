#!/bin/bash
# script was designed for Ubuntu
# based on https://esp-idf.readthedocs.io/en/v2.0/linux-setup.html

# get necessary software
sudo apt-get install git wget make libncurses-dev flex bison gperf python python-serial

# setup working directory
dir_name=esp32
wd=~/$dir_name

mkdir $wd || exit 1

cd $wd

# download correct file corresponding to system architecture
arch=$(uname -m)

if [ $arch == "x86_64" ]
then
    echo "0"
    file=xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

elif [ $arch == "i686" ] || [ $arch == "i386" ]
then
    echo "1"
    file=xtensa-esp32-elf-linux32-1.22.0-80-g6c4433a-5.2.0.tar.gz

else
    >&2 echo "system not supported"
    exit
fi

wget "https://dl.espressif.com/dl/$file" || exit 1

tar -xzf $file && rm $file

echo "# ESP32" >> ~/.profile
echo "export PATH=\"\$PATH:\$HOME/$dir_name/xtensa-esp32-elf/bin\"" >> ~/.profile

# setup esp-idf
git clone --recursive https://github.com/espressif/esp-idf.git
git submodule add https://github.com/tuanpmt/espmqtt.git components/espmqtt
echo "export IDF_PATH=\"\$HOME/$dir_name/esp-idf\"" >> ~/.profile

echo "You need to log out to make the changes effective"
