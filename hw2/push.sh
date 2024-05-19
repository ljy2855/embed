#!/bin/bash

cd app
arm-none-linux-gnueabi-gcc -static -o fpga_test_timer.out fpga_test_timer.c 

cd ..
cd driver
make clean
make

cd ..
adb push app/*.out /data/local/tmp
adb push driver/*.ko /data/local/tmp
adb push insmod.sh /data/local/tmp