#ifndef IO_H
#define IO_H
#include "protocol.h"

io_protocol preprocess_io(char key);
void process_value(char *current_input, int key);
void init_device();
void cleanup_device();
void run_motor();
void print_fnd(char *value);
void print_lcd(char *line1, char *line2);
void control_led(unsigned char led1, unsigned char led2, unsigned char all);
void kill_led();
char read_input();
#endif