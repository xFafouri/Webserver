#!/bin/bash

# Set variables
NGINX_VERSION=1.25.5
INSTALL_DIR=$HOME/nginx
PORT=8080

# Step 1: Download and extract Nginx
echo "Downloading Nginx..."
cd ~
wget http://nginx.org/download/nginx-$NGINX_VERSION.tar.gz -O nginx.tar.gz
tar -xzf nginx.tar.gz
cd nginx-$NGINX_VERSION

# Step 2: Configure, make, and install Nginx locally
echo "Configuring Nginx to install in $INSTALL_DIR..."
./configure --prefix=$INSTALL_DIR > /dev/null
make > /dev/null
make install > /dev/null

# Step 3: Update config to listen on unprivileged port
echo "Updating config to use port $PORT..."
sed -i "s/listen       80;/listen       $PORT;/" $INSTALL_DIR/conf/nginx.conf

# Step 4: Start Nginx
echo "Starting Nginx on port $PORT..."
$INSTALL_DIR/sbin/nginx

# Step 5: Confirm and test
sleep 1
curl -I http://localhost:$PORT

echo -e "\n✅ Nginx is running without sudo at: http://localhost:$PORT"
echo "To stop it, run: $INSTALL_DIR/sbin/nginx -s stop"
