#!/bin/bash

git submodule update --init --recursive

cd rpi_firmware/opt/
sudo cp -r * /opt
cd ../

cd ../avc2ts
./preinstall.sh || { echo 'AVC2TS preinstall failed' ; exit 1; }
make || { echo 'avc2ts build failed' ; exit 1; }
sudo cp avc2ts /usr/local/bin

cd ../librist
sudo apt install -y libcmocka-dev libmbedcrypto3 libmbedtls-dev libmbedtls12 liblz4-dev meson ninja-build pkg-config cmake libcjson-dev liblz4-1
sudo apt --fix-broken -y install || { echo 'apt install --fix-broken failed' ; exit 1; }
mkdir build_dir
cd build_dir
meson .. || { echo 'meson config failed' ; exit 1; }
ninja
sudo ninja install || { echo 'ninja  failed' ; exit 1; }
sudo ldconfig
cd ..

cd ../tsduck
sudo apt install libsrt1.4-openssl
sudo dpkg -i $(ls tsduck*) || { echo 'dpkg tsduck failed' ; exit 1; }
sudo apt-get install -f -y || { echo 'apt-get install failed after tsduck installation' ; exit 1; }
sudo dpkg -i $(ls hides*) || { echo 'dpkg hides failed' ; exit 1; }
sudo apt-get install -f -y || { echo 'apt-get install failed after hides installation' ; exit 1; }
sudo apt --fix-broken -y install || { echo 'apt install --fix-broken failed' ; exit 1; }

read -p "Press enter to restart the machine"

sudo reboot now
