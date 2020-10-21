#!/usr/bin/gnuplot

set terminal pngcairo size 525,393 noenhanced font "Verdana,10"
set output "contour.png"

# CSV file with ; as delimiter
set datafile separator ';'

set title 'Feature contour plot'
set ylabel 'Amplitude'
set xlabel 'Time (Frames)'

set key autotitle columnheader

plot '../../prosody.csv' using 2:3 with lines, \
     ''                  using 2:5 with lines
