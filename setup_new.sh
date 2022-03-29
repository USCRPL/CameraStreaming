#!/bin/bash

cd avc2ts
sudo apt install -y  libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libavdevice-dev libvncserver-dev

./preinstall.sh || { echo 'AVC2TS preinstall failed' ; exit 1; }
make || { echo 'avc2ts build failed' ; exit 1; }
sudo cp avc2ts /usr/local/bin

# cd ../rist-installer
# cd deb
# sudo ./install-prerequisites.sh
# sudo ./build.sh
# cd ../installers
# sudo dpkg -i $(ls librist*) || echo 'dpkg librist failed;'
# cd ..

# cd ../tsduck
# sudo apt install libsrt1.4-openssl dkms libelf-dev linux-headers
# sudo dpkg -i $(ls tsduck*) || { echo 'dpkg tsduck failed' ; exit 1; }
# sudo apt-get install -f -y || { echo 'apt-get install failed after tsduck installation' ; exit 1; }
# sudo dpkg -i $(ls hides*) || { echo 'dpkg hides failed' ; exit 1; }
# sudo apt-get install -f -y || { echo 'apt-get install failed after hides installation' ; exit 1; }
# sudo apt --fix-broken -y install || { echo 'apt install --fix-broken failed' ; exit 1; }

# read -p "Press enter to restart the machine"

# sudo reboot now
