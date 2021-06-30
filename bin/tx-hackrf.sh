#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <callsign>"
    exit 1
fi

# the callsign is passed as an argument
CALLSIGN=$1

# get location from gpsd and compose an APRS info message
MSG=`gps/gps-aprs.py`
echo $MSG

# generate PCM audio and play it locally
./aprs -c $CALLSIGN -o pkt.pcm -f pcm $MSG
play -r 48000 -c 1 -t raw -e floating-point -b 32 pkt.pcm

# generate IQ file and transmit with HackRF
./aprs -c $CALLSIGN -o pkt.s8 -f s8 $MSG
hackrf_transfer -f 144800000 -s 2400000 -t pkt.s8 -a 1 -x 40
