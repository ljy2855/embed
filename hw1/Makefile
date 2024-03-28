CC = arm-none-linux-gnueabi-gcc
OBJS = message.o shmem.o

message.o : message.h message.c
	$(CC) -c message.c

shmem.o : shmem.h shmem.c
	$(CC) -c shmem.c

main : main.c $(OBJS)
	$(CC) -o main main.c $(OBJS)

clean :
	rm main shmem.o message.o