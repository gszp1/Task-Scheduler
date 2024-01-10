#include "task_scheduler.h"

int main(int argc, char* argv[]) {
    
    struct mq_attr server_msg_queue_attributes;
    server_msg_queue_attributes.mq_maxmsg = MAX_MESSAGES;
    server_msg_queue_attributes.mq_flags = 0;
    server_msg_queue_attributes.mq_msgsize = 100 * sizeof(char); //temporary

    //try to create main queue
    mqd_t server_msg_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_EXCL | O_RDWR, 0666, &server_msg_queue_attributes);
    //program flow - true -> client | false -> server
    if ((server_msg_queue == -1) && (errno == EEXIST)) {
        
    } else {
        while (1) {

        }
        mq_close(server_msg_queue);
        mq_unlink(SERVER_QUEUE_NAME);
    }
    
}
