#!/bin/bash
# script was designed for Mac OS
# based on https://esp-idf.readthedocs.io/en/v2.0/macos-setup.html

###############################################################################
##### Set up of Toolchain for Mac OS
###############################################################################
printf "\n 
########### ########## ##########      ### ##########  ###########
 ##         ##          ##      ##      #   ##      ##  ##
 #######    ##########  #########  ###  #   ##      ##  #######
 ##                 ##  ##              #   ##      ##  ##
##########  ##########  ##             ### ##########  ####


   
                         #       #               ######### #########
                         ##     ##               #       # #       
      ###### ######      # #   # #  ##### #####  #       # #########
      #    # #    #      #  # #  # #    # #      #       #         #
      ###### #    #      #   #   #  ### # #####  ######### #########
                     
				  
				  
				    	   
                                 ###
                               ####
                               ###
                        #######   #######
                      #####################
                     ####################
                     ###################
                     ###################
                     ####################
                      #####################
                       ###################
                         ###############
                     	   ####   ####
					  
					  
"
printf "\nSetting up the ESP32 Toolchain for Mac OS..."

printf "\nInstalling prerequisits..."

# install pip
sudo easy_install pip

# install pyserial
sudo pip install pyserial

printf "\nChecking if MacPorts or Homebrew is installed..."
macports=port
homebrew=brew

install_macports="sudo port install gsed gawk binutils gperf grep gettext wget libtool autoconf automake"
install_homebrew="brew install gnu-sed gawk binutils gperftools gettext wget help2man libtool autoconf automake"

if which $macports >/dev/null; then
	while true; do
	    read -rep '\nMacPorts is installed.\nDo you wish to install the dependencies via MacPorts?\n' yn
		case $yn in
	        [Yy]* ) $install_macports; printf "\nGetting necessary software...\n"; break;;
	        [Nn]* ) echo exiting...; exit;;
	        * ) echo "Please answer yes or no.";;
	    esac
	done
elif which $homebrew >/dev/null; then
	install_brew="brew install wget"

	while true; do
		read -rep $'\nHomebrew is installed.\nDo you wish to install the dependencies via homebrew?\n' yn
		case $yn in
	        [Yy]* ) $install_homebrew; printf "\nGetting necessary software...\n"; break;;
	        [Nn]* ) echo exiting...; exit;;
	        * ) echo "Please answer yes or no.";;
	    esac
	done
else
    echo "Neither MacPorts nor homebrew is installed. Please install one of the two package managers in order to proceed."
	exit
fi

printf "\nSetting up working directory...\n"
dir_name=esp32
wd=~/$dir_name

mkdir $wd

cd $wd

#TODO:Download the latest version of the ESP32 file without knowing the explicite file name
printf "\nDownloading ESP32 file for mac...\n"  
file=xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz
wget "https://dl.espressif.com/dl/$file"

tar -xzf $file && rm $file

printf "\nSetting up esp-idf...\n"
git clone --recursive https://github.com/espressif/esp-idf.git
git -C esp-idf checkout v3.0.3
git -C esp-idf submodule update --init --recursive

printf "\nUpdating ~/.bash_profile...\n"
echo "" >> ~/.bash_profile
echo "# ESP32" >> ~/.bash_profile
echo "export PATH=\$HOME/$dir_name/xtensa-esp32-elf/bin:\$PATH" >> ~/.bash_profile
echo "export IDF_PATH=\$HOME/$dir_name/esp-idf" >> ~/.bash_profile

source ~/.bash_profile

printf "\nSetting up the ESP32 environment for Mac OS has been finished.\n"