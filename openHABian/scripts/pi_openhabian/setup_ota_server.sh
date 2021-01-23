
#install nginx
sudo apt install nginx
#replace SERVER_IP in ota.conf with IP address, the certs are alredy in the resource dir
sed -i -e "s/SERVER_IP/$HOST_IPV4/g" "$RES_DIR/nginx/conf.d/ota.conf"
#move config file
sudo cp "$RES_DIR/nginx/conf.d/ota.conf" /etc/nginx/conf.d
#restart nginx
sudo service nginx restart
