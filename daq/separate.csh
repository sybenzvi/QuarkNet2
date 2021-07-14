#!/bin/csh
#
# this script separates into downward upward noise and corrupted data
#   assuming three counters into channels #1, #2, #3 with #1 on top and
#   #3 on bottom
#
egrep '53 02|55 04|56 04|57 04' muonLifetime.txt >! downward.txt
egrep '53 01|55 01|56 02|57 01' muonLifetime.txt >! upward.txt
egrep '53 04|55 02|56 01|57 02' muonLifetime.txt >! noise.txt
egrep -v '53 02|55 04|56 04|57 04|53 01|55 01|56 02|57 01|53 04|55 02|56 01|57 02' muonLifetime.txt >! garbled.txt
