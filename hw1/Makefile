# CC = arm-none-linux-gnueabi-gcc
CC = gcc
OBJS = message.o shmem.o store.o

message.o : message.h message.c
	$(CC) -c message.c

shmem.o : shmem.h shmem.c
	$(CC) -c shmem.c

store.o : store.h store.c
	$(CC) -c store.c

main : main.c $(OBJS)
	$(CC) -o main main.c protocol.h $(OBJS)

clean :
	rm main shmem.o message.o store.o