### Push driver, binary file

1. `sh push.sh` (in host)

build driver, user code and push to device by adb

2. `cd /data/local/tmp` (in device)

move to working directory

3. `sh insmod.sh` (in device)

insmod drvier and make device file

### Run

`./fpga_test_timer` (in device)
