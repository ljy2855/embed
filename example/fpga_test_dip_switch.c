#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

volatile sig_atomic_t quit = 0;

void user_signal1(int sig) {
    quit = 1;
}

int main(void) {
    int dev;
    unsigned char dip_sw_buff = 0;

    dev = open("/dev/fpga_dip_switch", O_RDONLY);
    if (dev < 0) {
        perror("Device open error");
        return -1;
    }

    // Ensure the file is set to blocking mode
    int flags = fcntl(dev, F_GETFL, 0);
    fcntl(dev, F_SETFL, flags & ~O_NONBLOCK);

    signal(SIGINT, user_signal1);
    printf("Press <ctrl+c> to quit. \n");

    while(!quit) {
        usleep(400000);
        if (read(dev, &dip_sw_buff, 1) > 0) {
            printf("Read dip switch: 0x%02X \n", dip_sw_buff);
        }
    }

    close(dev);
    return 0;
}
