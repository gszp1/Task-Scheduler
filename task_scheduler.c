#include "task_scheduler.h"

// main function //

int main(int argc, char* argv[], char* envp[]) {
    // main server queue attributes
    struct mq_attr server_msg_queue_attributes;
    server_msg_queue_attributes.mq_maxmsg = MAX_MESSAGES;
    server_msg_queue_attributes.mq_flags = 0;
    server_msg_queue_attributes.mq_msgsize = sizeof(transfer_object_t); //temporary

    // try to create main queue
    mqd_t server_msg_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_EXCL | O_RDONLY, 0666, &server_msg_queue_attributes);
    if ((server_msg_queue == -1) && (errno == EEXIST)) {
        printf("Running process as client.\n");

        server_msg_queue  = mq_open(SERVER_QUEUE_NAME, O_WRONLY, 0666, &server_msg_queue_attributes);
        if (server_msg_queue == -1) {
            printf("Failed to connect with server queue.\n");
            return 1;
        }

        if (queue_send_arguments(argc, argv, server_msg_queue)) {
            printf("Failed to send arguments.\n");
            return 2;
        }

        mq_close(server_msg_queue);
    } else {
        printf("Running process as server.\n");

        tasks_list_t* tasks_list = NULL;
        if (task_list_init(&tasks_list)) {
            printf("Server failed to start. Terminating execution.\n");
            mq_close(server_msg_queue);
            mq_unlink(SERVER_QUEUE_NAME);
            return 0;
        }

        pid_t current_pid = getpid();
        while (1) {
            transfer_object_t transfer_object;
            mq_receive(server_msg_queue, (char*)(&transfer_object), sizeof(transfer_object_t), NULL);
            if (transfer_object.pid != current_pid) {
                current_pid = transfer_object.pid;
                create_new_task(tasks_list, transfer_object.content, current_pid);
            } else {
                if (strcmp("", transfer_object.content)) {
                    
                } else {
                    add_data_to_task(tasks_list, transfer_object.pid, transfer_object.content);
                }
            }
            printf("%s\n", transfer_object.content);
        }// temporary, change to proper requests handling later
        tasks_list_destroy(tasks_list);
        mq_close(server_msg_queue);
        mq_unlink(SERVER_QUEUE_NAME);
    }
    return 0;
}