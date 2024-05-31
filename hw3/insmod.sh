#!/bin/bash

insmod stopwatch_driver.ko
mknod /dev/stopwatch c 242 0