#!/bin/bash

cd app
make clean
make

cd ..
cd module
make clean
make

cd ..
adb push app/*.out /data/local/tmp
adb push module/*.ko /data/local/tmp
adb push insmod.sh /data/local/tmp