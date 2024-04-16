#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "protocol.h"

#define MAX_MESSAGE_SIZE 80

struct msgbuf
{
    long mtype;
    io_protocol data;
};
/**
 * initalize message queue and return key
 */
int init_message_queue();
/**
 * read queue and return io_protocol data
 */
void receive_message(int message_id, io_protocol *protocol);

/**
 * write queue with io_protocol data
 */
void send_message(int message_id, io_protocol *protocol);