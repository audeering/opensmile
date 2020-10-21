#!/usr/bin/gnuplot

set terminal pngcairo size 525,393 noenhanced font "Verdana,10"
set output "chroma.png"

set macros

# Ignore the first line and the first two columns of the data file
DATA='"<tail -n +2 ../../chroma.csv | cut --delimiter='';'' --fields=3- -"'

set palette gray
set yrange [-0.5:11.5]
set xlabel 'Time (frames)'
set ylabel 'Semitone bins'
set title 'Chromagram'
unset key

# csv file with ; as delimiter
set datafile separator ';'

# Get number of frames and set xrange accordingly
stats @DATA nooutput
set xrange [0:STATS_records]
set yrange [-0.5:10.5]

# Transpose data for plotting
plot @DATA u 2:1:3 matrix with image
