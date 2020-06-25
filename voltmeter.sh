#!/bin/bash


f=${f:-measurements-$(date --iso=minute | cut -d+ -f1)}

# Configure tty
stty -F /dev/ttyUSB0 -raw -icrnl -onlcr -echo

# Give the measure command
printf "measure\r\n" >/dev/ttyUSB0

# Set up reading from tty
cat /dev/ttyUSB0 | ts '%.s' | tee "$f" &

# Start streaming the data
# 1593109832.335000 1484 1695 1371 39750 2804

tail -f "$f" | awk '{ print $3/10000, $4/1000, 0, 3; fflush(stdout); }' \
	| bearplot &

trap 'jobs -p | xargs kill' EXIT
wait
