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
        // temporary reading and printing program arguments.
        server_msg_queue  = mq_open(SERVER_QUEUE_NAME, O_WRONLY, 0666, &server_msg_queue_attributes);
        if (server_msg_queue == -1) {
            printf("Failed to connect with server queue.");
            return -1;
        }
        for (int i = 1; i < argc; ++i) {
            printf("%s\n", argv[i]);
            transfer_object_t transfer_object;
            int counter = 0;
            while (argv[i][counter] != '\0' && counter < MAX_ARGUMENT_SIZE) {
                transfer_object.content[counter] = argv[i][counter];
                ++counter;
            }
            transfer_object.content[counter] = '\0';
            transfer_object.pid = getpid();
            printf("%d\n", mq_send(server_msg_queue, (char*)(&transfer_object), sizeof(transfer_object_t), 0));
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
            transfer_object_t transfer_object;
            mq_receive(server_msg_queue, (char*)(&transfer_object), sizeof(transfer_object_t), NULL);
            printf("%s\n", transfer_object.content);
        }// temporary, change to proper requests handling later
        tasks_list_destroy(tasks_list);
        mq_close(server_msg_queue);
        mq_unlink(SERVER_QUEUE_NAME);
    }
    return 0;
}

// functions definitions //

int task_list_init(tasks_list_t* tasks_list) {
    if (tasks_list != NULL) { // list was already initialized.
        return 1;
    }
    tasks_list = malloc(sizeof(tasks_list_t));
    if (tasks_list == NULL) { // failed to allocate memory to list
        return 1;
    }
    tasks_list->head = NULL;
    tasks_list->tail = NULL;
    tasks_list->max_id = 0;
    pthread_mutex_init(&(tasks_list->list_access_mutex), NULL);
    return 0;
}

void tasks_list_destroy(tasks_list_t* tasks_list) {
    if (tasks_list == NULL) {
        return;
    }
    // todo: remove nodes and all resources allocated by them.
    pthread_mutex_destroy(&(tasks_list->list_access_mutex));
    free(tasks_list);
}