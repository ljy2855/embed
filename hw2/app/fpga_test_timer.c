#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <asm/ioctl.h>

#define MAJOR_NUM 242
#define DEVICE_NAME "/dev/dev_driver"

#define SET_OPTION _IOW(MAJOR_NUM, 0, unsigned long)
#define COMMAND 1

int main(int argc, char **argv)
{
	int dev, i;
	int interval, cnt, initial_value;
	unsigned long data;

	if (argc != 4)
	{
		printf("Usage: ./fpga_test_timer TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]\n");
		return -1;
	}

	dev = open(DEVICE_NAME, O_WRONLY);
	if (dev < 0)
	{
		printf("Open Failured!\n");
		return -1;
	}

	interval = atoi(argv[1]);
	if (interval < 1 || interval > 100)
	{
		printf("Invalid TIMER_INTERVAL (1~100) !\n");
		close(dev);
		return -1;
	}
	cnt = atoi(argv[2]);
	if (cnt < 1 || cnt > 100)
	{
		printf("Invalid TIMER_CNT (1~100) !\n");
		close(dev);
		return -1;
	}
	initial_value = atoi(argv[3]);

	data = 0;

	// Encode data
	data = (initial_value & 0x3FFF) | ((cnt & 0x7F) << 14) | ((interval & 0x7F) << 21);

	ioctl(dev, SET_OPTION, data);
	ioctl(dev, COMMAND, data);

	close(dev);
	return 0;
}
