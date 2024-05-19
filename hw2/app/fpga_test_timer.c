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
	int intv, cnt, init;
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

	intv = atoi(argv[1]);
	if (intv < 1 || intv > 100)
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
	init = atoi(argv[3]);

	data = 0;
	// 비트 마스킹과 이동을 통한 데이터 조합
	for (i = 0; i < 4; i++)
	{
		data |= ((init >> (i * 4)) & 0xF) << (12 - 4 * i); // 4비트씩, 각 4자리에 배치
	}
	data |= ((uint64_t)(cnt & 0xFF) << 20);				  // cnt는 8비트 사용, 20번째 비트 위치부터 시작
	data |= ((uint64_t)(intv & 0xFF) << 32); // intv는 상위 8비트에 배치

	ioctl(dev, SET_OPTION, data);
	ioctl(dev, COMMAND, data);

	close(dev);
	return 0;
}
