#!/bin/bash

kill -9 $(pgrep -f './coordinator' | head -1)

kill -9 $(pgrep -f './server -i 1 -m 9190 -t master' | head -1)
kill -9 $(pgrep -f './server -i 1 -m 9490 -t slave' | head -1)

kill -9 $(pgrep -f './server -i 2 -m 9290 -t master' | head -1)
kill -9 $(pgrep -f './server -i 2 -m 9590 -t slave' | head -1)

kill -9 $(pgrep -f './server -i 3 -m 9390 -t master' | head -1)
kill -9 $(pgrep -f './server -i 3 -m 9690 -t slave' | head -1)

kill -9 $(pgrep -f './synchronizer -i 1 -m 9790' | head -1)
kill -9 $(pgrep -f './synchronizer -i 2 -m 9890' | head -1)
kill -9 $(pgrep -f './synchronizer -i 3 -m 9990' | head -1)

rm -rf logfiles/*
rm -rf userfiles/*