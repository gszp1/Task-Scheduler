#include "task_scheduler.h"

// global variables and mutexes //

pthread_mutex_t task_access_mutex = PTHREAD_MUTEX_INITIALIZER;

// main function //

int main(int argc, char* argv[]) {
    // main server queue attributes
    struct mq_attr server_msg_queue_attributes;
    server_msg_queue_attributes.mq_maxmsg = MAX_MESSAGES;
    server_msg_queue_attributes.mq_flags = 0;
    server_msg_queue_attributes.mq_msgsize = 256 * sizeof(char); //temporary

    // try to create main queue
    mqd_t server_msg_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_EXCL | O_RDONLY, 0666, &server_msg_queue_attributes);
    if ((server_msg_queue == -1) && (errno == EEXIST)) {
        printf("Running process as client.\n");
        // temporary reading and printing program arguments.
        server_msg_queue  = mq_open(SERVER_QUEUE_NAME, O_WRONLY, 0666, &server_msg_queue_attributes);
        if (server_msg_queue == -1) {
            return -1;
        }
        for (int i = 1; i < argc; ++i) {
            printf("%s\n", argv[i]);
            printf("%d\n", mq_send(server_msg_queue, argv[i], 256 * sizeof(char), 0));
        }
        mq_close(server_msg_queue);
    } else {
        printf("Running process as server.\n");

        tasks_list_t* tasks_list;
        if (task_list_init(tasks_list)) {
            printf("Server failed to start. Terminating execution.");
            mq_close(server_msg_queue);
            mq_unlink(SERVER_QUEUE_NAME);
            return 0;
        }

        while (1) {
            char msg[256];
            mq_receive(server_msg_queue, msg, 256 * sizeof(char), NULL);
            printf("%s\n", msg);
        }// temporary, change to proper requests handling later
        mq_close(server_msg_queue);
        mq_unlink(SERVER_QUEUE_NAME);
    }
    return 0;
}

// functions definitions //

int task_list_init(tasks_list_t* tasks_list) {
    tasks_list = malloc(sizeof(tasks_list_t));
    if (tasks_list = NULL) {
        return 1;
    }
    tasks_list->head = NULL;
    tasks_list->tail = NULL;
    tasks_list->max_id = 0;
    return 0;
}