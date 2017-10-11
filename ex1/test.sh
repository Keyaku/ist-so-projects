#!/bin/sh

time ./heatSim 1024 10 10 0 0 100 > single.out
time ./heatSim 1024 10 10 0 0 100 32 0 > multi.out

diff single.out multi.out

rm -f *.out
