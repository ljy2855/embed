#include "io.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <poll.h>

#define MAX_LENGTH 5
#define FPGA_STEP_MOTOR_DEVICE "/dev/fpga_step_motor"
#define FPGA_TEXT_LCD_DEVICE "/dev/fpga_text_lcd"
#define FPGA_READKEY "/dev/input/event0"
#define FPGA_PUSH_SWITCH "/dev/fpga_push_switch"
#define FND_DEVICE "/dev/fpga_fnd"
#define FPGA_DIP_SWITCH "/dev/fpga_dip_switch"

#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16 
#define MAX_BUFF 32
#define LINE_BUFF 16
int dev_motor;
int dev_lcd;
int dev_readkey;
int dev_switch;
int dev_fnd;
int dev_dip_switch;
int led_fd;

pid_t led_pid;

enum input_type{
    PRESS_SWITCH,
    PRESS_READ_KEY,
    PRESS_RESET,
};

struct pollfd input_fds[3];

unsigned char *led_addr =0;

io_protocol preprocess_io(char key)
{
    io_protocol io_data;
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

bool num_mode = true; // 숫자 모드인지 확인 (true면 숫자, false면 문자)
int last_key = 0;     // 마지막으로 누른 키 저장
int press_count = 0;  // 같은 키를 연속으로 누른 횟수

// 문자 모드에서 각 키에 해당하는 문자 순환을 위한 정보
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
    int len = strlen(current_input);

    if (key == 1)
    {
        num_mode = !num_mode; // 모드 전환
        last_key = 0;         // 키 정보 초기화
        press_count = 0;      // 키 누르기 횟수 초기화
        return;               // 모드 전환 키는 문자를 추가하지 않음
    }

    if (num_mode)
    {
        // 숫자 모드일 때
        if (key >= 2 && key <= 9)
        {
            if (len < MAX_LENGTH)
            {
                current_input[len] = '0' + key; // 숫자 입력
                current_input[len + 1] = '\0';  // 문자열 종료
            }
        }
        last_key = 0;    // 키 정보 초기화
        press_count = 0; // 키 누르기 횟수 초기화
    }
    else
    {
        // 문자 모드일 때
        if (key >= 2 && key <= 9)
        {
            if (last_key == key && len > 0)
            {
                // 같은 키를 눌렀다면, 순환
                
                    press_count = (press_count + 1) % strlen(key_map[key]);
                    current_input[len - 1] = key_map[key][press_count]; // 마지막 문자 업데이트
               
            }
            else
            {
                // 다른 키를 처음 눌렀다면 또는 문자열 길이가 0일 때
                if (len < MAX_LENGTH)
                {
                    current_input[len] = key_map[key][0]; // 첫 문자 입력
                    current_input[len + 1] = '\0';        // 문자열 종료
                    press_count = 0;                      // 키 누르기 횟수 초기화
                }
            }
        }
        last_key = key; // 현재 키 업데이트
    }
}

void init_device()
{
  
    unsigned long *fpga_addr = 0;
    // init device driver
    dev_motor = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);
    assert(dev_motor >= 0);
    dev_lcd = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
    assert(dev_lcd >= 0);
    dev_readkey =  open(FPGA_READKEY,O_RDONLY);
    assert(dev_readkey >= 0);
    dev_switch = open(FPGA_PUSH_SWITCH,O_RDWR);
    assert(dev_switch >= 0);
    dev_fnd = open(FND_DEVICE,O_RDWR);
    assert(dev_fnd >= 0);
    dev_dip_switch = open(FPGA_DIP_SWITCH,O_RDWR);
    assert(dev_dip_switch >= 0);

    input_fds[SWITCH].fd = dev_switch;
    input_fds[SWITCH].events = POLLIN;
    input_fds[READ_KEY].fd = dev_readkey;
    input_fds[READ_KEY].events = POLLIN;
    input_fds[RESET].fd = dev_dip_switch;
    input_fds[RESET].events = POLLIN;

    // init led mmap
    led_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (led_fd < 0) {
		perror("/dev/mem open error");
		
	}

	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led_fd, FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED)
	{
		printf("mmap error!\n");
		close(led_fd);
		
	}
	
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);
}

void run_motor(){
    unsigned char motor_state[3];
    memset(motor_state,0,sizeof(motor_state));
    motor_state[0] =1; // start
    motor_state[2] =10; // set speed
    write(dev_motor,motor_state,3);
    sleep(1);
    motor_state[0] =0;
    write(dev_motor,motor_state,3);
}

void print_fnd(char * value){
    unsigned char data[4];
    int i;
    for(i = 0 ; i < 4 ; i++)
        data[i] = value[i] -'0';
    write(dev_fnd,&data,4);
}

void print_lcd(char * line1, char * line2){
    unsigned char string[32];
    int str_size;
    memset(string,0,sizeof(string));
    str_size=strlen(line1);
	if(str_size>0) {
		strncat(string,line1,str_size);
		memset(string+str_size,' ',LINE_BUFF-str_size);
	}

	str_size=strlen(line2);
	if(str_size>0) {
		strncat(string,line2,str_size);
		memset(string+LINE_BUFF+str_size,' ',LINE_BUFF-str_size);
	}
    write(dev_lcd,string,MAX_BUFF);	
}



int read_input(){
    int ret;
    int i;
    ssize_t count;
    ret = poll(input_fds, 3, -1); // 무한 대기
    if (ret == -1) {
        // handle_error("poll failed");
    }

    for (i = 0; i < 3; i++) {
        if (input_fds[i].revents & POLLIN) {
            switch (i)
            {
            case PRESS_SWITCH:
                /* code */
                break;
            
            case PRESS_READ_KEY:
                /* code */
                break;
            case PRESS_RESET:
                /* code */
                break;
            }
            // ssize_t count = read(input_fds[i].fd, buffer, BUFFER_SIZE - 1);
            if (count == -1) {
                // handle_error("read failed");
            }
        }
    }
  

    return 0;
}