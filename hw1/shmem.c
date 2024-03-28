#include "shmem.h"

struct sembuf p1 = {0, -1, SEM_UNDO};
struct sembuf  p2 = {1, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO}; 
struct sembuf  v2 = {1, 1, SEM_UNDO};

void getseg(struct databuf **p1, struct databuf **p2)
{
    if ((shm_id1 = shmget(SHARED_KEY1, sizeof(struct databuf), 0600 | IFLAGS)) == -1)
    {
        perror("error shmget\n");
        exit(1);
    }
    if ((shm_id2 = shmget(SHARED_KEY2, sizeof(struct databuf), 0600 | IFLAGS)) == -1)
    {
        perror("error shmget\n");
        exit(1);
    }
    if ((*p1 = (struct databuf *)shmat(shm_id1, 0, 0)) == ERR)
    {
        perror("error shmget\n");
        exit(1);
    }
    if ((*p2 = (struct databuf *)shmat(shm_id2, 0, 0)) == ERR)
    {
        perror("error shmget\n");
        exit(1);
    }
}

int getsem(void)
{
    semun x;
    x.val = 0;
    int id = -1;
    if ((id = semget(SEM_KEY, 2, 0600 | IFLAGS)) == -1)
        exit(1);
    if (semctl(id, 0, SETVAL, x) == -1)
        exit(1);
    if (semctl(id, 1, SETVAL, x) == -1)
        exit(1);
    return (id);
}
void remobj(void)
{
    if (shmctl(shm_id1, IPC_RMID, 0) == -1)
        exit(1);
    if (shmctl(shm_id2, IPC_RMID, 0) == -1)
        exit(1);
    if (semctl(sem_id, 0, IPC_RMID, 0) == -1)
        exit(1);
}

// reader – 파일 읽기 처리 (표준입력  공유 메모리)
void reader(int semid, struct databuf *buf1, struct databuf *buf2)
{
    for (;;)
    {
        buf1->d_nread = read(0, buf1->d_buf, SIZE);
        semop(semid, &v1, 1);
        semop(semid, &p2, 1);
        if (buf1->d_nread <= 0)
            return;
        buf2->d_nread = read(0, buf2->d_buf, SIZE);
        semop(semid, &v1, 1);
        semop(semid, &p2, 1);
        if (buf2->d_nread <= 0)
            return;
    }
}

void writer(int semid, struct databuf *buf1, struct databuf *buf2)
{
    for (;;)
    {
        semop(semid, &p1, 1);
        semop(semid, &v2, 1);
        if (buf1->d_nread <= 0)
            return;
        write(1, buf1->d_buf, buf1->d_nread);
        semop(semid, &p1, 1);
        semop(semid, &v2, 1);
        if (buf2->d_nread <= 0)
            return;
        write(1, buf2->d_buf, buf2->d_nread);
    }
}