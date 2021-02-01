There are two directories. Scripts in *local_machine* or meant for the
system where you are want to compile the esp32 project on. Scripts in
*pi_openhabian* are meant for the e.g. Raspberry Pi running with
openHABian.

To install openHABian on an SD card, please refer to the official
[documentation of openHAB][1] for instructions. After inserting the
SD card to the device and booting the system, you should modify the
configuration for SSH afterwards. Therefore you have to wait until the
setup process of openhHABian is finished and shut down the system. Then
plug in the SD card on your machine

- mount the SD card on a Linux system and open the partition *rootfs*.
- All following paths are related to *rootfs* on the SD card.
  - add your public key to /home/openhabian/.ssh/authorized_keys
  - open /etc/ssh/sshd_config and modify the following lines
    - change **#PermitRootLogin prohibit-password** to **PermitRootLogin no**
    - change **#PasswordAuthentication yes** to **PasswordAuthentication no**
    - change **#PermitEmptyPasswords no** to **PermitEmptyPasswords no**
  - save your changes and put the SD card back in to the Raspberry Pi to
    boot it
- now you can run *remote_setup_pi_openhabian.sh* to start the setup
  process for this project

[1]: https://www.openhab.org/docs/installation/openhabian.html#raspberry-pi-prepackaged-sd-card-image
