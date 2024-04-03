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

int init_message_queue();
void receive_message(int message_id, io_protocol *protocol);
void send_message(int message_id, io_protocol *protocol);