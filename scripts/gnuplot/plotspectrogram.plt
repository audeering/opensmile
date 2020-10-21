#!/usr/bin/gnuplot

set terminal pngcairo size 525,393 noenhanced font "Verdana,10"
set output "spectrogram.png"

set macros

# Ignore the first line and the first two columns of the data file
DATA='"<tail -n +2 ../../spectrogram.csv | cut --delimiter='';'' --fields=3- -"'

set palette gray negative
set cbrange [0:50]
set yrange [0:1024]
set xlabel 'Time (frames)'
set ylabel 'Spectral bins'
set format cb '%g dB'
set title 'Spectrogram'
unset key

# CSV file with ; as delimiter
set datafile separator ';'

# Get number of frames and set xrange accordingly
stats @DATA nooutput
set xrange [0:STATS_records]

# Transpose data for plotting
plot @DATA u 2:1:3 matrix with image
