#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define MAX_MESSAGE_SIZE 80

struct msgbuf
{
    long mtype;
    char mtext[MAX_MESSAGE_SIZE];
};

int init_message_queue();
void receive_message(int message_id, char *text);
void send_message(int message_id, char *text);