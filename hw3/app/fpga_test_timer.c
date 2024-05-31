#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <asm/ioctl.h>

#define DEVICE_NAME "/dev/stopwatch"

int main(int argc, char **argv)
{
	int fd;
	fd = open(DEVICE_NAME,O_RDWR);
	write(fd,"",1);
	close(fd);
	return 0;
}
