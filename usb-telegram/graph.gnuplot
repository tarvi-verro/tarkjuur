#!/usr/bin/gnuplot

set terminal png size 1280,480
set output '/tmp/_graph200.png'

set grid
set timefmt "%s"

set ylabel "Sensor measurement (V)"
set xlabel "Time"
set xdata time
set y2label "Uncalibrated Temperature (°C)"
set y2tics

# File to get data from
f = "../data-gathering/measurements-latest"

# Get the timestamp from latest data point
last = system("tail -n1 ".f." | cut -d' ' -f1")
window = system("echo $window")

# Filter data for last N days
d = "< tail -n+3 ".f." | awk '$1 >= ".last." - ".window."'"

# Temperature smoothing samples
set samples 200

plot \
	d using ($1 + 2*60*60):($3/1000) smooth csplines axis x1y2 title "Room temperature" lc rgb "#aa000000", \
	d using ($1 + 2*60*60):($4/1000) w lines title "Soil moisture"

