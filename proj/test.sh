#!/bin/sh

time ./heatSim 1024 10 10 0 0 100 > single.out
time ./heatSim 1024 10 10 0 0 100 4 0 > multi.out

diff single.out multi.out > result.diff
if [ $? -ne 0 ]; then
	echo "Check result.diff"
fi

rm -f *.out
