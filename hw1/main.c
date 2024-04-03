#include "message.h"
#include "shmem.h"
#include "store.h"
#include <unistd.h>
#include <stdio.h>

enum MODE
{
    GET,
    PUT,
    MERGE,
};
enum INPUT_MODE
{
    ENG,
    DIGIT,
};

int message_id;
int io_sem;
struct databuf *buf1, *buf2;
void io_process();
void main_process();
void merge_process();

int main(void)
{

    // intialize
    init_store();
    message_id = init_message_queue();
    sem_id = getsem(0);
    io_sem = getsem(1);
    getseg(&buf1, &buf2);

    if (fork() == 0)
    {
        // io process
        io_process();
    }
    else
    {
        if (fork() == 0)
            merge_process();
        main_process();
    }

    return 0;
}

void main_process()
{
    char command[MAX_MESSAGE_SIZE];
    int flag = 1;
    io_protocol io_data;
    while (flag)
    {
        receive_message(message_id, &io_data);

        if (io_data.input_type == READ_KEY && io_data.value == BACK)
        {
            break;
        }
    }
}

void io_process()
{
    char command[MAX_MESSAGE_SIZE];
    int flag = 1;
    char key;
    io_protocol io_data;
    while (flag)
    {
        scanf("%c", &key);
        switch (key) // TODO change
        {
        case '1':
            io_data.input_type = SWITCH;
            io_data.value = ONE;
            break;
        case '2':
            io_data.input_type = SWITCH;
            io_data.value = TWO;
            break;
        case '3':
            io_data.input_type = SWITCH;
            io_data.value = THREE;
            break;
        case '4':
            io_data.input_type = SWITCH;
            io_data.value = FOUR;
            break;
        case '5':
            io_data.input_type = SWITCH;
            io_data.value = FIVE;
            break;
        case '6':
            io_data.input_type = SWITCH;
            io_data.value = SIX;
            break;
        case '7':
            io_data.input_type = SWITCH;
            io_data.value = SEVEN;
            break;
        case '8':
            io_data.input_type = SWITCH;
            io_data.value = EIGHT;
            break;
        case '9':
            io_data.input_type = SWITCH;
            io_data.value = NINE;
            break;
        case 'B':
            io_data.input_type = READ_KEY;
            io_data.value = BACK;
            flag = 0;
            break;
        default:
            continue;
        }
        printf("send\n");
        send_message(message_id, &io_data);
    }

    // write_shm(sem_id,buf1,command);
}

void merge_process()
{
}