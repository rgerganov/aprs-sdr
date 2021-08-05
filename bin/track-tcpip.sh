#!/bin/bash

BIN_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
TMP_FILE="/tmp/aprs"
APRS_SERVER="euro.aprs2.net:8080"

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <callsign> <passcode>"
    exit 1
fi

CALLSIGN=$1
PASSCODE=$2

while [ 1 ]; do
    MSG=$($BIN_DIR/aprs-msg.py)
    echo $MSG
    cat << EOF > $TMP_FILE
user $CALLSIGN pass $PASSCODE
${CALLSIGN}>APRS,TCPIP*:$MSG
EOF
    curl --data-binary "@${TMP_FILE}" -H "Content-Type: application/octet-stream" $APRS_SERVER
    sleep 60
done
