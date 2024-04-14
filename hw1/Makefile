# CC = arm-none-linux-gnueabi-gcc
CC = gcc
OBJS = message.o shmem.o store.o io.o

message.o : message.h message.c protocol.h
	$(CC) -c message.c

shmem.o : shmem.h shmem.c protocol.h
	$(CC) -c shmem.c

store.o : store.h store.c protocol.h
	$(CC) -c store.c -lrt

io.o : io.h io.c protocol.h
	$(CC) -c io.c

main : main.c $(OBJS)
	$(CC) -o main main.c $(OBJS) -lrt

clean :
	rm main $(OBJS)