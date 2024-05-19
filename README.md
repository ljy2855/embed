### board disk mount
`mount -o rw,remount,size=6G /dev/mmcblk0p4 /data`

### Change log level
`echo "7 6 1 7" > /proc/sys/kernel/printk`