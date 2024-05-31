### Push driver, binary file

1. `sh push.sh` (in host)

build driver, user code and push to device by adb

2. `cd /data/local/tmp` (in device)

move to working directory

3. `sh insmod.sh` (in device)

insmod drvier and make device file
driver name : stopwatch_driver
major num : 242
### Run

`./fpga_test_timer TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]` (in device)
