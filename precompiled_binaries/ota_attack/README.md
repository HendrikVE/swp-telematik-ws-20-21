These binaries are provided for the OTA attack. Place the directories `esp32-1` and `esp32-2` in `/var/www/html` on the
OTA update server (nginx on the Raspberry Pi). Additionally create the following empty directories: `esp32-1/3` and `esp32-2/4`

The binaries have certificates compiled in and assume openHAB and nginx
to run on IP 192.168.1.1 (using the WiFi setup by the Raspberry Pi).

For further information on how to use these to run the demo please have  
a look at the wiki on page `Demonstration Procedure`.
