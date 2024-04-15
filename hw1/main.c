#include "message.h"
#include "shmem.h"
#include "store.h"
#include "io.h"
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

enum PUT_INPUT_MODE
{
    KEY,
    VALUE,
};
int message_id;
int exit_sem;
struct databuf *buf1, *buf2;
void io_process();
void main_process();
void merge_process();
void add_char_and_update(char *original, char new_char);

int main(void)
{

    // intialize
    init_store();
    init_device();
    message_id = init_message_queue();
    sem_id = getsem(0);
    exit_sem = getsem(0);
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
    char line1[17], line2[17];
    int flag = 1;
    io_protocol io_data;
    enum MODE mode = PUT;
    char key[5] = INITIAL_KEY;
    char value[10] = INITIAL_VALUE;
    enum PUT_INPUT_MODE put_input_mode = KEY;
    enum INPUT_MODE input_mode = ENG;
    merge_result result;
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
            
            flush();
            if (storage_cnt() == 3)
            {
                write_shm(sem_id, buf1, "M");
                P(exit_sem);
            }
            write_shm(sem_id, buf1, "B");
            
            break;
        }
        if (io_data.input_type == READ_KEY && io_data.value != PROG)
        {
            printf("change mode\n");
            // change mode
            strcpy(key, INITIAL_KEY);
            strcpy(value, INITIAL_VALUE);
            put_input_mode = KEY;
            input_mode = ENG;
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
            

            if (io_data.input_type == SWITCH && io_data.value == ONE_LONG)
            {
                strcpy(key, INITIAL_KEY);
                strcpy(value, INITIAL_VALUE);
                put_input_mode = KEY;
                input_mode = ENG;
            }
            else if (io_data.input_type == SWITCH && io_data.value == FOUR_SIX)
            {
                if (strcmp(key, INITIAL_KEY) == 0 || strcmp(value, INITIAL_VALUE) == 0)
                    break;
                put_pair(atoi(key), value);
                if (storage_cnt() == 3)
                {
                    write_shm(sem_id, buf1, "M");
                }
            }
            else if (io_data.input_type == RESET)
            {
                put_input_mode = (put_input_mode + 1) % 2;
            }
            else
            {
                // input key or value
                if (put_input_mode == KEY)
                {
                    // input key
                    if (io_data.value <= NINE)
                    {
                        add_char_and_update(key, io_data.value + '0');
                        printf("update key : %s\n", key);
                    }
                }
                else
                {
                    process_value(value, io_data.value);
                    printf("update value : %s\n", value);
                }
            }
            print_fnd(key);

            break;
        case GET:
            memset(line1,0,17);
            memset(line2,0,17);
            sprintf(line1,"GET MODE");
            if (io_data.input_type == SWITCH)
            {
                if (io_data.value <= NINE)
                {
                    add_char_and_update(key, io_data.value + '0');
                }
                printf("cur key: %s\n", key);
            }
            if (io_data.input_type == RESET)
            {
                // submit
                if (strcmp(key, INITIAL_KEY) == 0)
                    break;
                temp = get_pair(atoi(key));
                // printf("%d\n", atoi(key));
                if (temp.index == NOT_FOUND)
                {
                    // TODO PRINT ERROR
                    sprintf(line2,"Error");

                }
                else
                {
                    // TODO PRINT OUTPUT
                    sprintf(line2,"(%d, %d,%s)",temp.index,temp.key,temp.value);
                    printf("[%d] %d %s\n", temp.index, temp.key, temp.value);
                }
            }
            print_fnd(key);
            print_lcd(line1,line2);

            break;
        case MERGE:
            if (io_data.input_type == RESET)
            {
                if (storage_cnt() >= 2)
                {
                    run_motor();
                    result = merge();
                    sprintf(line1,"%d merged",result.cnt);
                    sprintf(line2,"%s generated",result.filename);
                    print_lcd(line1,line2);
                    // print result LCD
                    // TODO spin motor
                }
            }
            break;
        }
    }
}

void io_process()
{
    char key;
    io_protocol io_data;
    while (1)
    {
        scanf("%c%*c", &key);
        if (key == '\n')
            continue;
        io_data = preprocess_io(key);
        printf("send\n");
        send_message(message_id, &io_data);
        if (io_data.input_type == READ_KEY && io_data.value == BACK)
            break;
    }
}

void merge_process()
{
    char message[2];
    while (1)
    {
        read_shm(sem_id, buf1, message);

        if (message[0] == 'M')
        {
            printf("Background merge start\n");
            run_motor();
            merge();
            V(exit_sem);
        }

        if (message[0] == 'B')
        {
            exit(0);
        }
    }
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