#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE 1048576 // 2^20

unsigned long long int get_system_memory()
{
	long pages = sysconf(_SC_AVPHYS_PAGES);
	long page_size = sysconf(_SC_PAGESIZE);
	return pages * page_size;
}

int main(int argc, char **argv)
{
	unsigned long long int memory_size;
	long request_size;
	int fd;
	char input[SIZE] = {0};
	char output[SIZE];

	fd = open("/dev/mem_ctrl", O_RDWR);
	if(fd < 0){
		printf("Open Failured!\n");
		return -1;
	}

	request_size = SIZE - 1630;
	memory_size = get_system_memory();
	write(fd, &input, request_size);
	printf("Request Size : %lu, Diff System Memory: %llu \n", SIZE, memory_size - get_system_memory());

	off_t loc = lseek(fd, 0, SEEK_SET);
	read(fd, &output, request_size);

	request_size = SIZE + 1630;
	memory_size = get_system_memory();
	write(fd, &input, request_size);
	printf("Request Size : %lu, Diff System Memory: %llu \n", SIZE, memory_size - get_system_memory());

	off_t loc = lseek(fd, 0, SEEK_SET);
	read(fd, &output, request_size);
	close(fd);
	return 0;
}

