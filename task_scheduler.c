#include "task_scheduler.h"

int main(int argc, char* argv[]) {
    //try to create main queue
    mqd_t server_msg_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_EXCL | O_RDWR, 0666, NULL);
    //program flow - true -> client | false -> server
    if ((server_msg_queue == -1) && (errno == EEXIST)) {
        
    } else {

    }
    
}
