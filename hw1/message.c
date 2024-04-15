#include "message.h"

int init_message_queue()
{
    key_t key;
    int msgid;
    key = ftok("keyfile", 1);

    // 동일한 키를 가진 메시지 큐가 이미 존재하는지 확인
    msgid = msgget(key, IPC_CREAT | 0666);
    assert(msgid != -1);

    // 메시지 큐가 이미 존재한다면, 삭제
    if (msgid > -1)
    {
        if (msgctl(msgid, IPC_RMID, NULL) == -1)
        {
            perror("Failed to remove existing message queue");
            assert(0);
        }
    }

    // 새로운 메시지 큐 생성
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