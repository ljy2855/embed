#include "message.h"

int init_message_queue()
{
    key_t key;
    int msgid;
    key = ftok("keyfile", 1);

    // check message queue exist
    msgid = msgget(key, IPC_CREAT | 0666);
    assert(msgid != -1);

    // remove prior message queue
    if (msgid > -1)
    {
        if (msgctl(msgid, IPC_RMID, NULL) == -1)
        {
            perror("Failed to remove existing message queue");
            assert(0);
        }
    }

    // generate new message queue
    msgid = msgget(key, IPC_CREAT | 0666);
    assert(msgid != -1);

    return msgid;
}

void send_message(int message_id, io_protocol *protocol)
{
    struct msgbuf mesg;
    int flag = 0;

    mesg.mtype = 1;
    mesg.data = *protocol;
    flag = msgsnd(message_id, (void *)&mesg, sizeof(io_protocol), 0);

    assert(flag != -1);
}

void receive_message(int message_id, io_protocol *protocol)
{
    struct msgbuf mesg;
    int flag;

    flag = msgrcv(message_id, &mesg, sizeof(io_protocol), 1, 0);
    assert(flag != -1);
    *protocol = mesg.data;
}