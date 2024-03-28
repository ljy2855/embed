#define _POSIX_UNION_SEMUN

#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>

#define SHARED_KEY1 (key_t)0x10 /* 공유 메모리 키 */
#define SHARED_KEY2 (key_t)0x15 /*공유 메모리 키 */
#define SEM_KEY (key_t)0x20     /* 세마포 키 */
#define IFLAGS (IPC_CREAT)
#define ERR ((struct databuf *)-1)
#define SIZE 2048

struct sembuf p1 = {0, -1, SEM_UNDO}, p2 = {1, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO}, v2 = {1, 1, SEM_UNDO};
struct databuf
{ /* data 와 read count 저장 */
    int d_nread;
    char d_buf[SIZE];
};

typedef union 
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} semun;

static int shm_id1, shm_id2, sem_id;