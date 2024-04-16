#include "io.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/input.h>

#define MAX_LENGTH 5
#define FPGA_STEP_MOTOR_DEVICE "/dev/fpga_step_motor"
#define FPGA_TEXT_LCD_DEVICE "/dev/fpga_text_lcd"
#define FPGA_READKEY "/dev/input/event0"
#define FPGA_PUSH_SWITCH "/dev/fpga_push_switch"
#define FND_DEVICE "/dev/fpga_fnd"
#define FPGA_DIP_SWITCH "/dev/fpga_dip_switch"

#define FPGA_BASE_ADDRESS 0x08000000 // fpga_base address
#define LED_ADDR 0x16
#define MAX_BUFF 32
#define LINE_BUFF 16
#define MAX_BUTTON 9
#define BUFF_SIZE 64

// device driver fds
int dev_motor;
int dev_lcd;
int dev_readkey;
int dev_switch;
int dev_fnd;
int dev_dip_switch;
int led_fd;

unsigned long *fpga_addr = 0; // mmap address to control led device
unsigned char *led_addr = 0;  // led mmap address
pid_t led_pid = 0;            // store current led process

enum input_type
{
    PRESS_SWITCH,
    PRESS_READ_KEY,
    PRESS_RESET,
};

io_protocol preprocess_io(char key)
{
    io_protocol io_data;
    switch (key)
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
    case 'L':
        // 1 long input
        io_data.input_type = SWITCH;
        io_data.value = ONE_LONG;
        break;
    case 'D':
        // 4 + 6 input
        io_data.input_type = SWITCH;
        io_data.value = FOUR_SIX;
        break;
    }
    return io_data;
}

// number pad mapping
char key_map[10][4] = {
    "",     // 0
    "",     // 1
    "abc",  // 2
    "def",  // 3
    "ghi",  // 4
    "jkl",  // 5
    "mno",  // 6
    "pqrs", // 7
    "tuv",  // 8
    "wxyz"  // 9
};

void process_value(char *current_input, int key)
{
    static bool num_mode = true;
    static int last_key = 0;
    static int press_count = 0;
    int len = strlen(current_input);

    if (key == 1)
    {
        num_mode = !num_mode; // convert input mode
        last_key = 0;         // init last key
        press_count = 0;      // init press cnt
        return;
    }

    if (num_mode)
    {
        // input number
        if (key >= 2 && key <= 9)
        {
            if (len < MAX_LENGTH)
            {
                current_input[len] = '0' + key;
                current_input[len + 1] = '\0';
            }
        }
        last_key = 0;
        press_count = 0;
    }
    else
    {
        // input character
        if (key >= 2 && key <= 9)
        {
            if (last_key == key && len > 0)
            {
                // circulate next character

                press_count = (press_count + 1) % strlen(key_map[key]);
                current_input[len - 1] = key_map[key][press_count];
            }
            else
            {
                // new character input
                if (len < MAX_LENGTH)
                {
                    current_input[len] = key_map[key][0];
                    current_input[len + 1] = '\0';
                    press_count = 0;
                }
            }
        }
        last_key = key;
    }
}

void init_device()
{

    // init device driver
    dev_motor = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);
    assert(dev_motor >= 0);
    dev_lcd = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
    assert(dev_lcd >= 0);
    dev_readkey = open(FPGA_READKEY, O_RDONLY | O_NONBLOCK);
    assert(dev_readkey >= 0);
    dev_switch = open(FPGA_PUSH_SWITCH, O_RDWR);
    assert(dev_switch >= 0);
    dev_fnd = open(FND_DEVICE, O_RDWR);
    assert(dev_fnd >= 0);
    dev_dip_switch = open(FPGA_DIP_SWITCH, O_RDWR);
    assert(dev_dip_switch >= 0);

    // init led mmap
    led_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (led_fd < 0)
    {
        perror("/dev/mem open error");
    }

    fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led_fd, FPGA_BASE_ADDRESS);
    if (fpga_addr == MAP_FAILED)
    {
        printf("mmap error!\n");
        close(led_fd);
    }

    led_addr = (unsigned char *)((void *)fpga_addr + LED_ADDR);
}

void cleanup_device()
{
    // close device fd
    if (dev_motor >= 0)
        close(dev_motor);
    if (dev_lcd >= 0)
        close(dev_lcd);
    if (dev_readkey >= 0)
        close(dev_readkey);
    if (dev_switch >= 0)
        close(dev_switch);
    if (dev_fnd >= 0)
        close(dev_fnd);
    if (dev_dip_switch >= 0)
        close(dev_dip_switch);

    // munmap
    if (fpga_addr != MAP_FAILED)
    {
        if (munmap((void *)fpga_addr, 4096) == -1)
        {
            printf("munmap led_addr failed");
        }
    }

    // close led fd
    if (led_fd >= 0)
        close(led_fd);
}

void run_motor()
{
    unsigned char motor_state[3];
    memset(motor_state, 0, sizeof(motor_state));
    motor_state[0] = 1;  // start
    motor_state[2] = 10; // set speed
    write(dev_motor, motor_state, 3);
    sleep(1);
    motor_state[0] = 0; // stop
    write(dev_motor, motor_state, 3);
}

void print_fnd(char *value)
{
    unsigned char data[4];
    int i;
    for (i = 0; i < 4; i++)
        data[i] = value[i] - '0';
    write(dev_fnd, &data, 4);
}

void print_lcd(char *line1, char *line2)
{
    unsigned char string[32];
    int str_size;
    memset(string, 0, sizeof(string));
    str_size = strlen(line1);
    if (str_size > 0)
    {
        strncat(string, line1, str_size);
        memset(string + str_size, ' ', LINE_BUFF - str_size);
    }

    str_size = strlen(line2);
    if (str_size > 0)
    {
        strncat(string, line2, str_size);
        memset(string + LINE_BUFF + str_size, ' ', LINE_BUFF - str_size);
    }
    write(dev_lcd, string, MAX_BUFF);
}

void control_led(unsigned char led1, unsigned char led2, unsigned char all)
{
    if (led_pid != 0)
    {
        if (kill(led_pid, 0) == 0)
        {                              // Check if the process exists
            kill(led_pid, SIGKILL);    // Terminate the existing process
            waitpid(led_pid, NULL, 0); // Ensure the process is cleaned up
            printf("Killed existing process with PID %d\n", led_pid);
        }
        led_pid = 0; // Reset global PID
    }

    led_pid = fork();
    if (led_pid == 0)
    { // Child process
        if (all)
        {
            *led_addr = 0xff;
            usleep(500000);
            *led_addr = 0;
        }
        while (1)
        {
            *led_addr = 128 / led1;
            usleep(500000); // 500 milliseconds
            *led_addr = 128 / led2;
            usleep(500000); // 500 milliseconds
        }
        return;
    }
    else if (led_pid > 0)
    { // Parent process
        printf("Started blinking process with PID %d\n", led_pid);
    }
    else
    {
        perror("Failed to fork");
    }
}
void kill_led()
{
    if (led_pid != 0)
    {
        if (kill(led_pid, 0) == 0)
        {                              // Check if the process exists
            kill(led_pid, SIGKILL);    // Terminate the existing process
            waitpid(led_pid, NULL, 0); // Ensure the process is cleaned up
            printf("Killed existing process with PID %d\n", led_pid);
        }
        led_pid = 0; // Reset global PID
    }
}
/**
 * Check start_time over 1 sec
 */
bool has_one_second_elapsed(struct timeval start_time)
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - start_time.tv_sec >= 2)
    {
        return true;
    }
    return false;
}

char read_input()
{
    struct timeval start_time;
    int ret;
    int i, j;
    unsigned char push_sw_buff[MAX_BUTTON];
    unsigned char dip_sw_buff = 0;
    struct input_event ev[BUFF_SIZE];
    int size = sizeof(struct input_event);

    // get reset button
    if (read(dev_dip_switch, &dip_sw_buff, 1) > 0)
    {
        if (!dip_sw_buff)
            return 'R';
    }
    // get switch input
    if (read(dev_switch, &push_sw_buff, sizeof(push_sw_buff)) > 0)
    {
        for (j = 0; j < MAX_BUTTON; j++)
        {

            if (push_sw_buff[j])
            {
                if (j == 3 && push_sw_buff[5])
                {
                    return 'D'; // Some specific command
                }
                if (j == 0)
                { // Assuming button '1' is at index 0
                    gettimeofday(&start_time, NULL);

                    while (!has_one_second_elapsed(start_time))
                    {
                        read(dev_switch, &push_sw_buff, sizeof(push_sw_buff));
                        if (push_sw_buff[0] == 0)
                            return '1';
                        usleep(10000);
                    }
                    return 'L';
                }
                return '1' + j; // Immediate return for any button press
            }
        }
    }

    // get read_key input
    if (read(dev_readkey, ev, size * BUFF_SIZE) > 0)
    {
        if (ev[0].type == EV_KEY && ev[0].value == 1)
        { // Key press event
            switch (ev[0].code)
            {
            case KEY_BACK:
                return 'B';
            case KEY_VOLUMEUP:
                return '+';
            case KEY_VOLUMEDOWN:
                return '-';
            }
        }
    }

    usleep(10000);
    return 0; // No input detected
}