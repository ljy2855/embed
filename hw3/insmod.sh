#!/bin/bash

insmod new_timer_driver.ko
mknod /dev/dev_driver c 242 0