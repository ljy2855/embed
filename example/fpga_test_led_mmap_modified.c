#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16 

int main(int argc, char **argv)
{
	int fd,i;

	unsigned long *fpga_addr = 0;
	unsigned char *led_addr =0;
	unsigned char data;

	if(argc!=2) {
		printf("please input the parameter! \n");
		printf("ex)./test_led [0-255]\n");
		return -1;
	}
	data=atoi(argv[1]);  //ex : digit 5 -> 0000 0101

	if( (data<0) || (data>255) ){
		printf("Invalid range!\n");
		exit(1);
	}

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("/dev/mem open error");
		exit(1);
	}

	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED)
	{
		printf("mmap error!\n");
		close(fd);
		exit(1);
	}
	
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);
	
	*led_addr=data; //write led
	
	sleep(1);
	//for read
	data=0;
	data=*led_addr; //read led
	printf("Current LED VALUE : %d\n",data);

	munmap(led_addr, 4096);
	close(fd);
	return 0;
}
