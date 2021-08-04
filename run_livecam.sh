#!/bin/sh
#
# Copy the avc2ts binary and this script to /usr/local/bin, and add
# the following line to /etc/rc.local:
#
#   /usr/local/bin/run_livecam.sh &
#

trap 'kill $BGPID; exit' INT

TARGET_IP=127.0.0.1
TARGET_PORT=4000

##VIDEO SETTINGS##
SERVICENAME="USC RPL DOMEPIERCER LIVESTREAM "
WIDTH=1024
HEIGHT=768
FRAMERATE=25
VIDEORATE=3100000
TSRATE=3400000

##TRANSMISSION SETTINGS##
FREQUENCY=426000000  # in Hz
BANDWIDTH=8          # in MHz
CONSTELLATION=16-QAM #Constellation type. Must be one of QPSK, 16-QAM, 64-QAM.
#GAIN=6
GAIN=-25			 #Adjust the output gain to the specified value in dB.
GUARDINTERVAL=1/32   #Guard interval. Must be one of 1/32, 1/16, 1/8, 1/4.
HIGHPRIORITYFEC=2/3  #Error correction for high priority streams. Must be one of 1/2, 2/3, 3/4, 5/6, 7/8.

#remaining settings (currently using defaults):
##HiDes adapter number
##dc-compensation
##HiDes device name
##Spectral inversion
##transmission mode


avc2ts -s $SERVICENAME \
    -x $WIDTH -y $HEIGHT -f $FRAMERATE \
    -b $VIDEORATE -m $TSRATE \
    -n $TARGET_IP:$TARGET_PORT &

BGPID=$!

tsp --realtime -I ip --local-address $TARGET_IP $TARGET_PORT \
        -O hides -f $FREQUENCY -b $BANDWIDTH -c $CONSTELLATION \
		--gain $GAIN -g $GUARDINTERVAL --high-priority-fec $HIGHPRIORITYFEC