#include "message.h"

int init_message_queue()
{
    key_t key;
    int msgid;
    key = ftok("keyfile", 1);
    msgid = msgget(key, IPC_CREAT | 0666);
    assert(msgid != -1);

    return msgid;
}

void receive_message(int message_id, char *text)
{
    struct msgbuf mesg;
    int flag = 0;

    assert(strlen(text) < MAX_MESSAGE_SIZE);

    mesg.mtype = 1;
    strncpy(mesg.mtext, text, MAX_MESSAGE_SIZE);
    flag = msgsnd(message_id, (void *)&mesg, MAX_MESSAGE_SIZE, 0);

    assert(flag != -1);
    printf("message send sucess\n");
}

void send_message(int message_id, char *text)
{
    struct msgbuf mesg;
    int flag = 0;

    mesg.mtype = 1;

    flag = msgrcv(message_id, &mesg, MAX_MESSAGE_SIZE, 0, 0);
    assert(flag != -1);
    strncpy(text, mesg.mtext, MAX_MESSAGE_SIZE);
    printf("message recv sucess '%s'\n", text);
}