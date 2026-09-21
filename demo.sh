#!/usr/bin/env bash
# runs both sides of the timing channel for a quick round trip.
# usage: ./demo.sh "message to send"
set -e

MSG="${1:-chishie fishie sussies quanti}"

make >/dev/null

./receiver &
RECV=$!
sleep 0.5              # give the receiver time to bind before we start sending

./sender "$MSG"
wait "$RECV"
