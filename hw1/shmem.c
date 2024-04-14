#include "shmem.h"

struct sembuf p1 = {0, -1, SEM_UNDO};
struct sembuf p2 = {1, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO};
struct sembuf v2 = {1, 1, SEM_UNDO};

void init_segment(int shm_id, struct databuf **ptr)
{
    *ptr = (struct databuf *)shmat(shm_id, NULL, 0);
    if (*ptr == ERR)
    {
        perror("shmat error");
        exit(1);
    }

    // Forcefully initialize memory to ensure cleanliness
    memset((*ptr)->d_buf, 0, SIZE);
    (*ptr)->d_nread = 0;
}

void clear_shm(int shm_id)
{
    void *temp_ptr = shmat(shm_id, NULL, 0);
    if (temp_ptr == ERR)
    {
        perror("shmat error for cleanup");
        return;
    }
    if (shmdt(temp_ptr) != 0)
    {
        perror("shmdt failed during cleanup");
        return;
    }
    if (shmctl(shm_id, IPC_RMID, NULL) != 0)
    {
        perror("shmctl IPC_RMID failed");
    }
}

void getseg(struct databuf **p1, struct databuf **p2)
{
    int shm_id1, shm_id2;

    // Try to access the shared memory, if it exists clear it and recreate
    shm_id1 = shmget(SHARED_KEY1, sizeof(struct databuf), 0600);
    if (shm_id1 >= 0)
    {
        clear_shm(shm_id1);
    }
    shm_id1 = shmget(SHARED_KEY1, sizeof(struct databuf), 0600 | IPC_CREAT | IPC_EXCL);
    if (shm_id1 == -1)
    {
        perror("error shmget for SHARED_KEY1");
        exit(1);
    }
    init_segment(shm_id1, p1);

    // Repeat for the second key
    shm_id2 = shmget(SHARED_KEY2, sizeof(struct databuf), 0600);
    if (shm_id2 >= 0)
    {
        clear_shm(shm_id2);
    }
    shm_id2 = shmget(SHARED_KEY2, sizeof(struct databuf), 0600 | IPC_CREAT | IPC_EXCL);
    if (shm_id2 == -1)
    {
        perror("error shmget for SHARED_KEY2");
        exit(1);
    }
    init_segment(shm_id2, p2);
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
void P(int sem_id)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;     // 세마포어 번호, 0이면 첫 번째 세마포어
    sem_b.sem_op = -1;     // P 연산, 세마포어 값을 1 감소
    sem_b.sem_flg = SEM_UNDO; // 시스템 종료 시 세마포어 자동 복구
    if (semop(sem_id, &sem_b, 1) == -1) {
        perror("semop P operation failed");
        exit(EXIT_FAILURE);
    }
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