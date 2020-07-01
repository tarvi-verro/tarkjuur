
f=${f:-measurements-$(date --iso=minute | cut -d+ -f1)}

# Cycle latest symbolic link
latest="measurements-latest"
rm "$latest" 2>/dev/null
ln -s "$f" "$latest"

# Configure tty
stty -F /dev/ttyUSB0 -raw -icrnl -onlcr -echo

# Reset by sending ^C
printf "\3" >/dev/ttyUSB0
sleep 0.1

# Give the measure command and skip the header line
printf "measure\r\n" >/dev/ttyUSB0
grep -q '^Vref' /dev/ttyUSB0

# Reading from tty
cat /dev/ttyUSB0 | ts '%.s' >> "$f"

