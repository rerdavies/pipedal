#!/usr/bin/bash
#!/bin/bash

# Create a temporary file to store the data
temp_file=$(mktemp)

# Create a temporary file to store the data
temp_file=$(mktemp)

# Run get_counts and process its output
./build/src/makeRelease --gplot-downloads | while read -r date count; do
    # Convert ISO8601 date to timestamp for gnuplot
    timestamp=$(date -d "$date" "+%s")
    echo "$timestamp $count" >> "$temp_file"
done

# Create and execute the gnuplot script
gnuplot -persist << EOF
set terminal qt enhanced font "Piboto,12" size 800,600
set xdata time
set timefmt "%s"
set format x "%Y-%m-%d"
set xlabel ""
set ylabel "Cumulative Downloads"
set title "PiPedal Downloads" font "Piboto:Bold,16"
set grid

# Rotate x-axis labels by 45 degrees
set xtics rotate by 90 offset 0,-4

# Adjust the bottom margin to make room for rotated labels
set bmargin 10

unset key

plot "$temp_file" using 1:2 with lines title "Cumulative downloads"


pause mouse close
EOF


# Clean up the temporary file
rm "$temp_file"

