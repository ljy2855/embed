### Run

1. `sh push.sh` (in host)
2. `cd /data/local/tmp` (in device)
2. `sh insmod.sh` (in device)
3. `./fpga_test_timer TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]` (in device)