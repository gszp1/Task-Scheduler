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
    if (strcmp(flag, "-a") == 0) {
        return ADD_TASK;
    }
    if (strcmp(flag, "-rm") == 0) {
        return REMOVE_TASK;
    }
    if (strcmp(flag, "-ls") == 0) {
        return LIST_TASKS;
    }
    return 0;
}

// Adds task to tasks list.
int add_task(task_t* task, tasks_list_t* tasks_list) {
    task_list_node_t* new_node = malloc(sizeof(task_list_node_t));
    if (new_node == NULL) {
        return 1;
    }
    new_node->task = task;
    new_node->next = NULL;
    pthread_mutex_lock(&(tasks_list->list_access_mutex));
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
    pthread_mutex_unlock(&(tasks_list->list_access_mutex));
    return 0;
}

// Adds data field read from queue to task.
int add_data_to_task(tasks_list_t* tasks_list, pid_t pid, data_field_t* data_field) {
    if (tasks_list == NULL) {
        return 1;
    }
    pthread_mutex_lock(&(tasks_list->list_access_mutex));
    task_list_node_t* node = tasks_list->head;
    while (node != NULL) {
        if (node->task->pid == pid) {
            break;
        }
        node = node->next;
    }
    if (node == NULL) {
        pthread_mutex_unlock(&(tasks_list->list_access_mutex)); 
        return 1;
    }
    if(node->task->data_fields == NULL) {
        node->task->data_fields = data_field;
    } else {
        data_field_t* current_field = node->task->data_fields;
        while (current_field->next_field != NULL) {
            current_field = current_field->next_field;
        }
        current_field->next_field = data_field;
    }
    node->task->number_of_fields += 1;
    pthread_mutex_unlock(&(tasks_list->list_access_mutex));
    return 0;
}

// Creates new task.
task_t* create_new_task(char* field, pid_t pid) {
    task_t* new_task = malloc(sizeof(task_t));
    new_task->pid = pid;
    new_task->number_of_fields = 1;
    data_field_t* data_field = malloc(sizeof(data_field_t));
    data_field->next_field = NULL;
    strcpy(data_field->data, field);
    new_task->data_fields = data_field;
    return new_task;
}

// Creates new data field.
data_field_t* create_data_field(char* data, pid_t pid) {
    if (data == NULL) {
        return NULL;
    }
    data_field_t* new_data_field = malloc(sizeof(data_field_t));
    if (new_data_field == NULL) {
        return NULL;
    }
    new_data_field->next_field = NULL;
    strcpy(new_data_field->data, data);
    return new_data_field;
}

// Removes task with given ID.
static int remove_task_by_id(tasks_list_t* tasks_list, unsigned long id) {
    if (tasks_list == NULL) {
        return 1;
    }
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
    return 0;
}

// message handlers //

// Handler for task removal query
static int remove_task_query_handler(tasks_list_t* tasks_list, task_list_node_t* task) {
    if ((tasks_list == NULL) || (task == NULL)) {
        return 1;
    }
    data_field_t* data_field = task->task->data_fields;
    unsigned long removed_task_id = 0;
    char read_fields = 0;
    while (data_field != NULL) {
        if (read_fields >= 2) {
            remove_task_by_id(tasks_list, task->task->id);
            return 1;
        }
        if (read_fields == 0) {
            if (get_query_type(data_field->data) != REMOVE_TASK) {
                remove_task_by_id(tasks_list, task->task->id);
                return 1;
            }
        } else {
            removed_task_id = strtoul(data_field->data, NULL, 10);
            if (removed_task_id == 0) {
                remove_task_by_id(tasks_list, task->task->id);
                return 1;
            }
        }
        ++read_fields;
        data_field = data_field->next_field;
    }
    remove_task_by_id(tasks_list, removed_task_id);
    remove_task_by_id(tasks_list, task->task->id);
    printf("Task removed.\n");
    return 0;
}

// Handler for task addition query
static int add_task_query_handler(tasks_list_t* tasks_list, task_list_node_t* task) {
    if ((tasks_list == NULL) || (task == NULL)) {
        return 1;
    }
    
}

// Handler for task listing query
static int list_tasks_query_handler(tasks_list_t* tasks_list, task_list_node_t* task) {
}

// Sets up and runs task.
int run_task(tasks_list_t* tasks_list, pid_t pid) {
    printf("Starting new task.");
    if (tasks_list == NULL) {
        return 1;
    }
    pthread_mutex_lock(&(tasks_list->list_access_mutex));
    task_list_node_t* current_node = tasks_list->head;
    while (current_node != NULL) {
        if (current_node->task->pid == pid) {
            break;
        }
        current_node = current_node->next;
    }
    if (current_node == NULL) {
        pthread_mutex_unlock(&(tasks_list->list_access_mutex));
        return 1;
    }
    data_field_t* data_field = current_node->task->data_fields;
    switch (get_query_type(data_field->data)) {
        case ADD_TASK:
            add_task_query_handler(tasks_list, current_node);
            break;
        case LIST_TASKS:
            list_tasks_query_handler(tasks_list, current_node);
            break;
        case REMOVE_TASK:
            remove_task_query_handler(tasks_list, current_node);
            break;
        default:
            pthread_mutex_unlock(&(tasks_list->list_access_mutex));
            return 2;
    }
    pthread_mutex_unlock(&(tasks_list->list_access_mutex));
    return 0;
}