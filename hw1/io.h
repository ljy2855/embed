#ifndef IO_H
#define IO_H
#include "protocol.h"
/**
 * generate io_protocol data from input key
 */
io_protocol preprocess_io(char key);

/**
 * update value string with input key and current mode
 */
void process_value(char *current_input, int key);

/**
 * initialize device drvier and shared mermory
 */
void init_device();

/**
 * close device drvier and shared memroy
 */
void cleanup_device();

/**
 * control motor interface
 */
void run_motor();

/**
 * control fnd display interface
 */
void print_fnd(char *value);

/**
 * control lcd display interface
 */
void print_lcd(char *line1, char *line2);

/**
 * control led display interface
 * fork and reaping child process that twinkle two pattern led on background
 */
void control_led(unsigned char led1, unsigned char led2, unsigned char all);

/**
 * kill led process
 */
void kill_led();

/**
 * get read_key, reset, switch input from each device
 */
char read_input();
#endif