#ifndef SHMEM_H
#define SHMEM_H

#define _POSIX_UNION_SEMUN

#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <string.h>

#define SHARED_KEY1 (key_t)0x11 /* 공유 메모리 키 */
#define SHARED_KEY2 (key_t)0x14 /*공유 메모리 키 */
#define SEM_KEY (key_t)0x20     /* 세마포 키 */
#define IFLAGS (IPC_CREAT)
#define ERR ((struct databuf *)-1)
#define SIZE 2

struct databuf
{ /* data 와 read count 저장 */
    int d_nread;
    char d_buf[SIZE];
};

typedef union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} semun;

static int shm_id1, shm_id2, sem_id;

/**
 * initalize two shared memory
 */
void getseg(struct databuf **p1, struct databuf **p2);
/**
 * get semaphore and return id
 */
int getsem(int val);
/**
 * lock with semaphore
 */
void P(int sem_id);
/**
 * unlock with semaphore
 */
void V(int sem_id);
/**
 * write shared memory data
 */
void write_shm(int semid, struct databuf *buf, char *data);
/**
 * read shared memory data
 */
void read_shm(int semid, struct databuf *buf, char *data);

#endif