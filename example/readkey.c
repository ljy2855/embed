#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1

int main (int argc, char *argv[])
{
	struct input_event ev[BUFF_SIZE];
	int fd, rd, value, size = sizeof (struct input_event);
	char name[256] = "Unknown";

	char* device = "/dev/input/event0";
	if((fd = open (device, O_RDONLY)) == -1) {
		printf ("%s is not a vaild device.n", device);
	}
	// ioctl (fd, EVIOCGNAME (sizeof (name)), name);
	// printf ("Reading From : %s (%s)n", device, name);

	while (1){
		if ((rd = read (fd, ev, size * BUFF_SIZE)) < size)
		{
			printf("read()");  
			return (0);     
		}

		value = ev[0].value;

		if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Only read the key press event
			printf ("code%d\n", (ev[1].code));
		}
		if( value == KEY_RELEASE ) {
			printf ("key release\n");
		} else 				if( value == KEY_PRESS ) {
			printf ("key press\n");
		}
		printf ("Type[%d] Value[%d] Code[%d]\n", ev[0].type, ev[0].value, (ev[0].code));
	}

	return 0;
}
