
f=${f:-measurements-$(date --iso=minute | cut -d+ -f1)}

# Cycle latest symbolic link
latest="measurements-latest"
rm "$latest" 2>/dev/null
ln -s "$f" "$latest"

# Configure tty
stty -F /dev/ttyUSB0 -raw -icrnl -onlcr -echo

# Give the measure command
printf "measure\r\n" >/dev/ttyUSB0

# Reading from tty
cat /dev/ttyUSB0 | ts '%.s' >> "$f"

