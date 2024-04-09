#include "message.h"
#include "shmem.h"
#include "store.h"
#include <unistd.h>
#include <stdio.h>

#define INITIAL_KEY "0000"
#define INITIAL_VALUE ""

enum MODE
{
    PUT,
    GET,
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
void add_char_and_update(char *original, char new_char);

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
        else
            main_process();
    }

    return 0;
}

void main_process()
{
    char command[MAX_MESSAGE_SIZE];
    int flag = 1;
    io_protocol io_data;
    enum MODE mode = PUT;
    char key[5] = INITIAL_KEY;
    char value[10] = INITIAL_VALUE;
    table temp;
    while (flag)
    {
        printf("cur mode : %d\n", mode);
        // TODO LCD OUTPUT
        receive_message(message_id, &io_data);

        if (io_data.input_type == READ_KEY && io_data.value == BACK)
        {
            // terminate program
            // TODO flush memory to storage
            break;
        }
        if (io_data.input_type == READ_KEY && io_data.value != PROG)
        {
            printf("change mode\n");
            // change mode
            strcpy(key, INITIAL_KEY);
            strcpy(value, INITIAL_VALUE);
            if (io_data.value == VOL_UP)
                mode = (mode + 1) % 3;
            else if (io_data.value == VOL_DOWN)
            {
                if (--mode == -1)
                    mode = MERGE;
            }
            continue;
        }
        switch (mode)
        {
        case PUT:
            break;
        case GET:
            printf("cur key: %s\n", key);
            if (io_data.input_type == SWITCH)
            {
                if (io_data.value <= NINE)
                {
                    add_char_and_update(key, io_data.value + '0');
                }
                // TODO reset key
            }
            if (io_data.input_type == READ_KEY)
            {
                // submit
                temp = get_pair(atoi(key));
                if (temp.index == NOT_FOUND)
                {
                    // TODO PRINT ERROR
                }
                else
                {
                    // TODO PRINT OUTPUT
                }
            }
            break;
        case MERGE:
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
        scanf("%c%*c", &key);
        if (key == '\n')
            continue;
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
        case 'R':
            io_data.input_type = RESET;
            break;
        case '+':
            io_data.input_type = READ_KEY;
            io_data.value = VOL_UP;
            break;
        case '-':
            io_data.input_type = READ_KEY;
            io_data.value = VOL_DOWN;
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

// helpers
void add_char_and_update(char *original, char new_char)
{
    static int next_index = 0; // 다음에 채울 인덱스를 저장하는 static 변수
    int length = 4;

    // 인덱스가 문자열 길이를 초과하는 경우, 처음부터 다시 시작합니다.
    if (next_index >= length)
    {
        next_index = 0; // 인덱스를 처음으로 리셋합니다.
    }

    // 다음 인덱스에 새로운 문자를 추가하고 인덱스를 증가시킵니다.
    original[next_index] = new_char;
    next_index++;
}