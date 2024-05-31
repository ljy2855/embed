CC := arm-none-linux-gnueabi-gcc


all: fpga_test_timer 

fpga_test_timer : 
	$(CC) -static -o fpga_test_timer.out  fpga_test_timer.c
	
clean : 
	rm -rf *.o
	rm -rf fpga_test_timer.out
