#include "shmem.h"

struct sembuf p1 = {0, -1, SEM_UNDO};
struct sembuf p2 = {1, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO};
struct sembuf v2 = {1, 1, SEM_UNDO};

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

int getsem(int val)
{
    semun x;
    x.val = val;
    int id = -1;
    if ((id = semget(SEM_KEY, 2, 0600 | IFLAGS)) == -1)
        exit(1);
    if (semctl(id, 0, SETVAL, x) == -1)
        exit(1);
    if (semctl(id, 1, SETVAL, x) == -1)
        exit(1);
    return (id);
}
void V(int sem_id)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1; // V 연산
    sem_b.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_b, 1);
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

void write_shm(int semid, struct databuf *buf, char *data)
{
    buf->d_nread = strlen(data);
    strncpy(buf->d_buf, data, buf->d_nread);

    semop(semid, &v1, 1);
    semop(semid, &p2, 1);
}

void read_shm(int semid, struct databuf *buf, char *data)
{

    semop(semid, &p1, 1);
    semop(semid, &v2, 1);
    if (buf->d_nread > 0)
    {
        strncpy(data, buf->d_buf, buf->d_nread);
        data[buf->d_nread] = '\0';
        buf->d_nread = 0;
    }
}

// void reader(int semid, struct databuf *buf1, struct databuf *buf2)
// {
//     for (;;)
//     {
//         buf1->d_nread = read(0, buf1->d_buf, SIZE);
//         semop(semid, &v1, 1);
//         semop(semid, &p2, 1);
//         if (buf1->d_nread <= 0)
//             return;
//         buf2->d_nread = read(0, buf2->d_buf, SIZE);
//         semop(semid, &v1, 1);
//         semop(semid, &p2, 1);
//         if (buf2->d_nread <= 0)
//             return;
//     }
// }

// void writer(int semid, struct databuf *buf1, struct databuf *buf2)
// {
//     for (;;)
//     {
//         semop(semid, &p1, 1);
//         semop(semid, &v2, 1);
//         if (buf1->d_nread <= 0)
//             return;
//         write(1, buf1->d_buf, buf1->d_nread);
//         semop(semid, &p1, 1);
//         semop(semid, &v2, 1);
//         if (buf2->d_nread <= 0)
//             return;
//         write(1, buf2->d_buf, buf2->d_nread);
//     }
// }