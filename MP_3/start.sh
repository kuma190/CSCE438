#!/bin/bash

./coordinator &
sleep 0.1

./server -i 1 -m 9190 -t master &
./server -i 1 -m 9490 -t slave &

./server -i 2 -m 9290 -t master &
./server -i 2 -m 9590 -t slave &

./server -i 3 -m 9390 -t master &
./server -i 3 -m 9690 -t slave &

./synchronizer -i 1 -m 9790 &
./synchronizer -i 2 -m 9890 &
./synchronizer -i 3 -m 9990 &
