#!/bin/bash

git submodule update --init --recursive


sudo apt-get update -qq && sudo apt-get -y install autoconf automake build-essential cmake git-core libass-dev libfreetype6-dev libgnutls28-dev libmp3lame-dev libsdl2-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev meson ninja-build pkg-config texinfo wget yasm zlib1g-dev nasm libx264-dev libx265-dev libnuma-dev libvpx-dev libfdk-aac-dev libopus-dev libdav1d-dev 

cd FFmpeg
./configure --prefix=`pwd`/install --enable-gpl --enable-nonfree --arch=aarch64 --enable-libass --enable-libfreetype --enable-libmp3lame --enable-libopus --enable-libvorbis --enable-libvpx --enable-libx264 --enable-libx265 > configure.out
make install

cd ../tsduck
sudo apt install libsrt1.4-openssl
sudo dpkg -i $(ls tsduck*) || { echo 'dpkg tsduck failed' ; exit 1; }
sudo apt-get install -f -y || { echo 'apt-get install failed after tsduck installation' ; exit 1; }
sudo dpkg -i $(ls hides*) || { echo 'dpkg hides failed' ; exit 1; }
sudo apt-get install -f -y || { echo 'apt-get install failed after hides installation' ; exit 1; }
sudo apt --fix-broken -y install || { echo 'apt install --fix-broken failed' ; exit 1; }

read -p "Press enter to restart the machine"

sudo reboot now
