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

        tasks_list_t* tasks_list;
        if (task_list_init(tasks_list)) {
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
            }
            printf("%s\n", transfer_object.content);
        }// temporary, change to proper requests handling later
        tasks_list_destroy(tasks_list);
        mq_close(server_msg_queue);
        mq_unlink(SERVER_QUEUE_NAME);
    }
    return 0;
}

// functions definitions //

// Initialize tasks linked list.
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

// Destroy tasks linked list.
void tasks_list_destroy(tasks_list_t* tasks_list) {
    if (tasks_list == NULL) {
        return;
    }
    // todo: remove nodes and all resources allocated by them.
    pthread_mutex_destroy(&(tasks_list->list_access_mutex));
    free(tasks_list);
}

// Send program arguments to server.
int queue_send_arguments(int argc, char* argv[], mqd_t message_queue) {
    transfer_object_t transfer_object;
    transfer_object.pid = getpid();
    for (int i = 1; i < argc; ++i) {
        int counter = 0;
        while (argv[i][counter] != '\0' && counter < MAX_ARGUMENT_SIZE) {
            transfer_object.content[counter] = argv[i][counter];
            ++counter;
        }
        transfer_object.content[counter] = '\0';
        if (mq_send(message_queue, (char*)(&transfer_object), sizeof(transfer_object_t), 0) == -1) {
            return 1;
        }
    }
    transfer_object.content[0] = '\0';
    return mq_send(message_queue, (char*)(&transfer_object), sizeof(transfer_object_t), 0);
}

// Checks if given string is a flag.
int get_query_type(char* flag) {
    if (strcmp(flag, "-a")) {
        return ADD_TASK;
    }
    if (strcmp(flag, "-rm")) {
        return REMOVE_TASK;
    }
    if (strcmp(flag, "-ls")) {
        return LIST_TASKS;
    }
    return 0;
}

// Removes task with given ID.
int remove_task_by_id(tasks_list_t* tasks_list, unsigned long id) {
    if (tasks_list == NULL) {
        return 1;
    }
    pthread_mutex_lock(&(tasks_list->list_access_mutex));
    task_list_node_t* current_node = tasks_list->head;
    while(current_node != NULL) {
        if (current_node->task->id == id) {
            // todo: stop and remove clock
            if ((current_node == tasks_list->tail) && (current_node == tasks_list->head)) {
                tasks_list->tail = NULL;
                tasks_list->head = NULL;
            } else {
                if (current_node->prev != NULL) {
                    current_node->prev->next = current_node->next;
                } else {
                    tasks_list->head = current_node->next;
                }

                if (current_node->next != NULL) {
                    current_node->next->prev = current_node->prev;
                } else {
                    tasks_list->tail = current_node->prev;
                }
            }
            free(current_node);
            break;
        }
        current_node = current_node->next;
    }
    pthread_mutex_unlock(&(tasks_list->list_access_mutex));
    return 0;
}

// Creates new task.
int create_new_task(tasks_list_t* tasks_list, char* field, pid_t pid) {
    task_t* new_task = malloc(sizeof(task_t));
    new_task->pid = pid;
    new_task->number_of_fields = 1;
    data_field_t* data_field = malloc(sizeof(data_field_t));
    data_field->next_field = NULL;
    strcpy(data_field->data, field);
    add_task(new_task, tasks_list);
    return 0;
}

int add_task(task_t* task, tasks_list_t* tasks_list) {
    task_list_node_t* new_node = malloc(sizeof(task_list_node_t));
    new_node->task = task;
    new_node->next = NULL;
    new_node->prev = tasks_list->tail;
    if (tasks_list->tail != NULL) {
        tasks_list->tail->next = new_node;
    }
    tasks_list->tail = new_node;
    if (tasks_list->head == NULL) {
        tasks_list->head = new_node;
    }
    (tasks_list->max_id)++;
    task->id = tasks_list->max_id;
    return 0;
}

// Finds task by pid.
task_list_node_t* find_task_by_pid(pid_t pid, tasks_list_t* tasks_list) {
    if (tasks_list == NULL) {
        return NULL;
    }
    task_list_node_t* current_node = tasks_list->head;
    while (current_node != NULL) {
        if (current_node->task->pid == pid) {
            return current_node;
        }
        current_node = current_node->next;
    }
    return NULL;
}

// Adds data field read from queue to task.
int add_data_to_task(tasks_list_t* tasks_list, pid_t pid, char* data) {
    if (tasks_list == NULL) {
        return 1;
    }
    task_list_node_t* node = find_task_by_pid(pid, tasks_list);
    if (node == NULL) {
        return 2;
    }
    data_field_t* data_field = malloc(sizeof(data_field_t));
    if (data_field == NULL) {
        return 3;
    }
    data_field->next_field = NULL;
    strcpy(data_field->data, data);
    if(node->task->data_fields == NULL) {
        node->task->data_fields = data_field;
    } else {
        node->task->data_fields->next_field = data_field;
    }
    node->task->number_of_fields += 1;
    return 0;
}