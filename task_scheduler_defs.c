#include "task_scheduler.h"

// functions definitions //

// Initialize tasks linked list.
int task_list_init(tasks_list_t** tasks_list) {
    if (*tasks_list != NULL) { // list was already initialized.
        return 1;
    }
    *tasks_list = malloc(sizeof(tasks_list_t));
    if (tasks_list == NULL) { // failed to allocate memory to list
        return 1;
    }
    (*tasks_list)->head = NULL;
    (*tasks_list)->tail = NULL;
    (*tasks_list)->max_id = 0;
    pthread_mutex_init(&((*tasks_list)->list_access_mutex), NULL);
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
    new_task->data_fields = data_field;
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