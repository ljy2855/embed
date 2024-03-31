#include "message.h"
#include "shmem.h"
#include "store.h"
#include <unistd.h>

int message_id;
struct databuf *buf1, *buf2;
void io_process();
void main_process();
void merge_process();

int main(void){

    // intialize
    init_store();
    message_id = init_message_queue();
    sem_id = getsem();
    getseg(&buf1,&buf2);


    if(fork() == 0){
        // io process
        io_process();
    }else{
        main_process();
        if(fork() == 0)
            merge_process();
    }

    

    return 0;
}

void main_process(){
    char command[MAX_MESSAGE_SIZE];
    // receive_message(message_id,command);
    read_shm(sem_id,buf1,command);
    printf("%s\n",command);
}

void io_process(){
    char command[MAX_MESSAGE_SIZE];
    scanf("%s",command);
    // send_message(message_id,command);
    write_shm(sem_id,buf1,command);



}

void merge_process(){

}