#include "message.h"
#include "shmem.h"
#include "store.h"
#include "io.h"
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
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
int merge_sem;
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

    // init message queue
    message_id = init_message_queue();
    sem_id = getsem(0);
    merge_sem = getsem(0);
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
/**
 * main process
 * 1. manage key value store
 * 2. get io data from io process
 * 3. send merge data to merge process
 */
void main_process()
{
    char line1[17], line2[17];
    int index = -1;
    io_protocol io_data;
    enum MODE mode = PUT;
    char key[5] = INITIAL_KEY;
    char value[10] = INITIAL_VALUE;
    enum PUT_INPUT_MODE put_input_mode = KEY;
    enum INPUT_MODE input_mode = ENG;
    merge_result result;
    table temp;

    // init device output
    control_led(1, 1, 0);
    print_fnd("0000");
    print_lcd("PUT MODE", "");
    while (1)
    {
        printf("cur mode : %d\n", mode);
        receive_message(message_id, &io_data);

        if (io_data.input_type == READ_KEY && io_data.value == BACK)
        {
            // terminate program
            flush();
            if (storage_cnt() == 3)
            {
                // send message to merge process
                write_shm(sem_id, buf1, "M");
                P(merge_sem);
            }
            // send message to terminate merge process
            write_shm(sem_id, buf1, "B");

            // kill led process
            kill_led();

            // close device driver
            cleanup_device();
            break;
        }
        if (io_data.input_type == READ_KEY && io_data.value != PROG)
        {
            // change mode

            // init state
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
            if (mode == GET)
            {
                control_led(1 << 4, 1 << 4, 0); // turn on led 5
            }
            else if (mode == PUT)
            {
                control_led(1, 1, 0); // turn on led 1
            }
            continue;
        }
        switch (mode)
        {
        case PUT:
            // init LCD output
            memset(line1, 0, 17);
            memset(line2, 0, 17);
            sprintf(line1, "PUT MODE");
            if (io_data.input_type == SWITCH && io_data.value == ONE_LONG)
            {
                strcpy(key, INITIAL_KEY);
                strcpy(value, INITIAL_VALUE);
                put_input_mode = KEY;
                input_mode = ENG;
                control_led(1, 1, 0); // turn on led 1
            }
            else if (io_data.input_type == SWITCH && io_data.value == FOUR_SIX)
            {
                if (strcmp(key, INITIAL_KEY) == 0 || strcmp(value, INITIAL_VALUE) == 0)
                    break;
                index = put_pair(atoi(key), value);
                sprintf(line2, "(%d, %s,%s)", index, key, value);
                control_led(1, 1, 1);
                if (storage_cnt() == 3)
                {
                    write_shm(sem_id, buf1, "M");
                    P(merge_sem);
                }
            }
            else if (io_data.input_type == RESET)
            {
                put_input_mode = (put_input_mode + 1) % 2;
                if (put_input_mode == KEY)
                    control_led(1 << 2, 1 << 3, 0); // 3,4 led twinkle
                else
                    control_led(1 << 6, 1 << 7, 0); // 7,8 led twinkle
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
                    control_led(1 << 2, 1 << 3, 0);
                }
                else
                {
                    process_value(value, io_data.value);
                    printf("update value : %s\n", value);
                }
                sprintf(line2, "%s", value);
            }
            print_fnd(key);
            print_lcd(line1, line2);

            break;
        case GET:
            // init LCD output
            memset(line1, 0, 17);
            memset(line2, 0, 17);
            sprintf(line1, "GET MODE");
            if (io_data.input_type == SWITCH)
            {
                // update key
                if (io_data.value <= NINE)
                {
                    add_char_and_update(key, io_data.value + '0');
                }
                printf("cur key: %s\n", key);
                control_led(1 << 2, 1 << 3, 0);
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
                    sprintf(line2, "Error");
                }
                else
                {
                    sprintf(line2, "(%d, %d,%s)", temp.index, temp.key, temp.value);
                    printf("[%d] %d %s\n", temp.index, temp.key, temp.value);
                }
                control_led(1 << 4, 1 << 4, 1);
            }
            print_fnd(key);
            print_lcd(line1, line2);

            break;
        case MERGE:
            if (io_data.input_type == RESET)
            {
                // request merge
                if (storage_cnt() >= 2)
                {
                    run_motor();
                    result = merge();
                    sprintf(line1, "%d merged", result.cnt);
                    sprintf(line2, "%s", result.filename);
                    print_lcd(line1, line2);
                }
            }
            break;
        }
    }
}
/**
 * process device input and send to main process
 */
void io_process()
{
    char key;
    io_protocol io_data;
    while (1)
    {
        key = read_input();
        // scanf("%c%*c",&key);
        if (key == 0)
            continue;
        io_data = preprocess_io(key);
        send_message(message_id, &io_data);
        // escape loop
        if (io_data.input_type == READ_KEY && io_data.value == BACK)
            break;
        usleep(500000);
    }
    cleanup_device();
}
/**
 * process background merge job
 * monitor shared memory change and process merge
 */
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
            V(merge_sem);
        }

        if (message[0] == 'B')
        {
            cleanup_device();
            exit(0);
        }
    }
}

// helpers
/**
 * update key with new digit
 */
void add_char_and_update(char *original, char new_char)
{
    static int next_index = 0; // store current update index
    int length = 4;

    if (next_index >= length)
    {
        next_index = 0;
    }

    original[next_index] = new_char;
    next_index++;
}