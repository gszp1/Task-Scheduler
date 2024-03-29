#include "task_scheduler.h"
#include "app_state_logger.h"

static int do_logs_flag = 0;

static pthread_mutex_t list_access_mutex;

///////////////////////////
// functions definitions //
//////////////////////////

////////////////////////////////////////
// linked list manipulation functions //
///////////////////////////////////////


// Initialize tasks linked list.
int task_list_init(tasks_list_t** tasks_list, int do_logs) {
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
    do_logs_flag = do_logs;
    if (do_logs == 1) {
        initialize_logger();
    }
    pthread_mutex_init(&list_access_mutex, NULL);
    return 0;
}

// frees resources used by task
void destroy_task(task_t* task) {
    data_field_t* data_field = task->data_fields;
    data_field_t* next_field = NULL;
    if (task->task_status == ACTIVE) {
        timer_delete(task->timer);
    }
    while (data_field != NULL) {
        next_field = data_field->next_field;
        free(data_field);
        data_field = next_field;
    }
    free(task);
}

// removes node from tasks list and frees resources.
void destroy_node(tasks_list_t* tasks_list, task_list_node_t* node) {
    if ((node == tasks_list->tail) && (node == tasks_list->head)) {
        tasks_list->tail = NULL;
        tasks_list->head = NULL;
    } else {
        if (node->prev != NULL) {
            node->prev->next = node->next;
        } else {
            tasks_list->head = node->next;
        }

        if (node->next != NULL) {
            node->next->prev = node->prev;
        } else {
            tasks_list->tail = node->prev;
        }
    }
    destroy_task(node->task);
    free(node);
}

// Destroy tasks linked list.
void tasks_list_destroy(tasks_list_t* tasks_list) {
    if (tasks_list == NULL) {
        return;
    }
    task_list_node_t* current_node = tasks_list->head;
    task_list_node_t* next_node = NULL;
    while (current_node != NULL) {
        next_node = current_node->next;
        destroy_task(current_node->task);
        free(current_node);
        current_node = next_node;
    }
    pthread_mutex_destroy(&list_access_mutex);
    free(tasks_list);
}

// Creates new task.
task_t* create_new_task(char* field, pid_t pid) {
    task_t* new_task = malloc(sizeof(task_t));
    if (new_task == NULL) {
        return NULL;
    }
    new_task->pid = pid;
    new_task->number_of_fields = 1;
    data_field_t* data_field = malloc(sizeof(data_field_t));
    if (data_field == NULL) {
        free(new_task);
        return NULL;
    }
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

// Adds data field read from queue to task.
int add_data_to_task(tasks_list_t* tasks_list, pid_t pid, data_field_t* data_field) {
    pthread_mutex_lock(&list_access_mutex);
    if (tasks_list == NULL) {
        pthread_mutex_unlock(&list_access_mutex);
        return 1;
    }
    task_list_node_t* node = tasks_list->head;
    while (node != NULL) {
        if (node->task->pid == pid) {
            break;
        }
        node = node->next;
    }
    if (node == NULL) {
        pthread_mutex_unlock(&list_access_mutex);
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
    pthread_mutex_unlock(&list_access_mutex);
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
    pthread_mutex_lock(&list_access_mutex);
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
    task->task_status = DISABLED;
    task->cyclic = 0;
    pthread_mutex_unlock(&list_access_mutex);
    return 0;
}

// Removes task with given ID.
static int remove_task_by_id(tasks_list_t* tasks_list, unsigned long id) {
    if (tasks_list == NULL) {
        return 1;
    }
    task_list_node_t* current_node = tasks_list->head;
    while(current_node != NULL) {
        if (current_node->task->id == id) {
            destroy_node(tasks_list, current_node);
            break;
        }
        current_node = current_node->next;
    }
    return 0;
}


///////////////////////////////
// task processing functions //
//////////////////////////////

// Function for creating logs.
static void create_log(char* command) {
    if (do_logs_flag == 0) {
        return;
    }
    time_t t;
    time(&t);
    struct tm *tm_info;
    tm_info = localtime(&t);
    unsigned long log_length = 21 + strlen(command);
    char* log = malloc(log_length * sizeof(char));
    if (log == NULL) {
        return;
    }
    strftime(log, 21, "%Y-%m-%d %H:%M:%S ", tm_info);
    strcat(log, command);
    write_to_login_file(log, STANDARD);
}

static char* create_log_contents(task_t* task, char* entry_text) {
    if (task == NULL || do_logs_flag == 0) {
        return NULL;
    }
    unsigned long length = (strlen(entry_text) + 1);
    char* content = malloc(length * sizeof(char));
    if (content == NULL) {
        return NULL;
    }
    char* safe_ptr = NULL;
    strcpy(content, entry_text);
    data_field_t* data_field = task->data_fields;
    while (data_field != NULL) {
        length += strlen(data_field->data) + 1;
        safe_ptr = realloc(content, length * sizeof(char));
        if (safe_ptr == NULL) {
            free(safe_ptr);
            free(content);
            return NULL;
        }
        content = safe_ptr;
        strcat(content, " ");
        strcat(content, data_field->data);
        data_field = data_field->next_field;
    }
    return content;
}

static void write_log(task_t* task, char* log_message) {
    if (log_message == NULL) {
        return;
    }
    char* log_contents = create_log_contents(task, log_message);
    if (log_contents == NULL) {
        create_log(log_message);        
    } else {
        create_log(log_contents);
        free(log_contents);
    }
}


/////////////////////////////
//     logs functions      //
/////////////////////////////

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

// Function for sending data to client through queue.
static int send_data_to_client(tasks_list_t* tasks_list, mqd_t client_queue) {
    if (tasks_list == NULL) {
        return 1;
    }
    client_transfer_object_t transfer_object;
    task_list_node_t* current_node = tasks_list->head;
    while (current_node != NULL) {
        data_field_t* data_field = current_node->task->data_fields;
        if (data_field == NULL) {
            transfer_object.last_record_entry = 1;
        } else {
            transfer_object.last_record_entry = 0;
        }
        sprintf(transfer_object.content, "%lu", current_node->task->id);
        if (mq_send(client_queue, (char*)(&transfer_object), sizeof(client_transfer_object_t), 0) == -1) {
            return 1;
        }
        if (current_node->task->task_status == ACTIVE) {
            sprintf(transfer_object.content, "ACTIVE |");
        } else {
            sprintf(transfer_object.content, "DISABLED |");
        }
        if (mq_send(client_queue, (char*)(&transfer_object), sizeof(client_transfer_object_t), 0) == -1) {
            return 1;
        }
        while (data_field != NULL) {
            strcpy(transfer_object.content,  data_field->data);
            if (data_field->next_field == NULL) {
                transfer_object.last_record_entry = 1;
            } else {
                transfer_object.last_record_entry = 0;
            }
            if (mq_send(client_queue, (char*)(&transfer_object), sizeof(client_transfer_object_t), 0) == -1) {
                return 1;
            }
            data_field = data_field->next_field;
        }

        current_node = current_node->next;
    }
    transfer_object.content[0] = '\0';
    if (mq_send(client_queue, (char*)(&transfer_object), sizeof(client_transfer_object_t), 0) == -1) {
        return 1;
    }
    return 0;
}

// task executed by thread called by timer.
void* timer_thread_task(void* arg) {
    pthread_mutex_lock(&list_access_mutex);
    timer_function_data_t* data = (timer_function_data_t*)arg;
    pid_t child_pid;
    data_field_t* data_field = data->task->task->data_fields;
    unsigned long read_fields = 0;
    unsigned long number_of_arguments = 0;
    char** arguments = malloc(sizeof(char*));
    if (arguments == NULL) {
        write_log(data->task->task, "Failed to start task:");
        destroy_node(data->tasks_list, data->task);
        pthread_mutex_unlock(&list_access_mutex);
        return NULL;
    }
    char** safe_ptr = NULL;
    while(data_field != NULL) {
        if (read_fields >= 3) {
            *(arguments + number_of_arguments) = data_field->data;
            ++number_of_arguments;
            safe_ptr = realloc(arguments, (number_of_arguments + 1) * sizeof(char*));
            if (arguments == NULL) {
                free(safe_ptr);
                free(arguments);
                write_log(data->task->task, "Failed to start task:");
                destroy_node(data->tasks_list, data->task);
                pthread_mutex_unlock(&list_access_mutex);
                return NULL;
            }
            arguments = safe_ptr;
        }
        ++read_fields;
        data_field = data_field->next_field;
    }
    safe_ptr = realloc(arguments, (number_of_arguments + 1) * sizeof(char*));
    if (arguments == NULL) {
        free(safe_ptr);
        free(arguments);
        write_log(data->task->task, "Failed to start task:");
        destroy_node(data->tasks_list, data->task);
        pthread_mutex_unlock(&list_access_mutex);
        return NULL;
    }
    arguments = safe_ptr;
    *(safe_ptr + number_of_arguments) = NULL;

    if (posix_spawnp(&child_pid, *arguments, NULL, NULL, arguments, *(data->envp)) != 0) {
        free(arguments);
        write_log(data->task->task, "Failed to start task:");
        destroy_node(data->tasks_list, data->task);
        pthread_mutex_unlock(&list_access_mutex);
        return NULL;
    }
    free(arguments);
    if (data->task->task->cyclic == 0) {
        write_log(data->task->task, "Removed task:");
        destroy_node(data->tasks_list, data->task);
    }
    pthread_mutex_unlock(&list_access_mutex);
    return NULL;
}


//////////////////////
// message handlers //
/////////////////////


// Handler for task removal query
static int remove_task_query_handler(tasks_list_t* tasks_list, task_list_node_t* task) {
    if ((tasks_list == NULL) || (task == NULL)) {
        return 1;
    }
    data_field_t* data_field = task->task->data_fields;
    unsigned long removed_task_id = 0;
    char read_fields = 0;
    unsigned long log_length = strlen("Finished command: -rm ");
    data_field_t* log_data_field = NULL;
    while (data_field != NULL) {
        if (read_fields >= 2) {
            return 1;
        }
        if (read_fields == 0) {
            if (get_query_type(data_field->data) != REMOVE_TASK) {
                return 1;
            }
        } else {
            removed_task_id = strtoul(data_field->data, NULL, 10);
            if (removed_task_id == 0) {
                return 1;
            }
            log_length += strlen(data_field->data);
            log_data_field = data_field;
        }
        ++read_fields;
        data_field = data_field->next_field;
    }
    write_log(task->task, "Finished task:");
    if (removed_task_id == task->task->id) {
        return 0;
    }
    remove_task_by_id(tasks_list, removed_task_id);
    return 0;
}

// Handler for task addition query
static int add_task_query_handler(tasks_list_t* tasks_list, task_list_node_t* task, char*** envp) {
    if ((tasks_list == NULL) || (task == NULL)) {
        return 1;
    }
    data_field_t* data_field = task->task->data_fields;
    if (data_field == NULL) {
        return 1;
    }
    unsigned long read_fields = 0;
    int time_type = 0; //relative / absoulute
    time_t time = 0;
    time_t repeat_time = 0;
    while(data_field != NULL) {
        switch (read_fields) {
            case 0:
                if (get_query_type(data_field->data) != ADD_TASK) {
                    return 1;
                }
                break;
            case 1:
                time = get_time(data_field->data, &time_type);
                if (time == -1) {
                    return 1;
                }
                break;
            case 2:
                repeat_time = convert_string_to_seconds(data_field->data);
                if (repeat_time == -1) {
                    return 1;
                }
                break;
        }
        ++read_fields;
        data_field = data_field->next_field;
    }
    if (read_fields < 4) {
        return 1;
    }
    if (repeat_time != 0) {
        task->task->cyclic = 1;
    }

    timer_function_data_t timer_data;
    timer_data.task = task;
    timer_data.tasks_list = tasks_list;
    timer_data.envp = envp;

    struct sigevent timer_event;
    timer_event.sigev_notify = SIGEV_THREAD;
    timer_event.sigev_notify_function = timer_thread_task;
    timer_event.sigev_value.sival_ptr = &timer_data;
    timer_event.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME, &timer_event, &(task->task->timer));

    time_t nsec = 0;
    if (time == 0 && repeat_time == 0) {
        nsec = 1;
    }
    struct itimerspec tispec;
    tispec.it_value.tv_sec = time;
    tispec.it_value.tv_nsec = nsec;
    tispec.it_interval.tv_sec = repeat_time;
    tispec.it_interval.tv_nsec = 0;
    task->task->task_status = ACTIVE;
    if(time_type == 0) {
        timer_settime(task->task->timer, 0, &tispec, NULL);
    } else {
        timer_settime(task->task->timer, TIMER_ABSTIME, &tispec, NULL);
    }
    write_log(task->task, "Finished starting task:");
    return 0;
}

// Handler for task listing query
static int list_tasks_query_handler(tasks_list_t* tasks_list, task_list_node_t* task) {
    if ((tasks_list == NULL) || (task == NULL)) {
        return 1;
    }
    char queue_name[32];
    sprintf(queue_name, "%s%d", USER_QUEUE_NAME, task->task->pid);
    struct mq_attr msg_queue_attributes;
    msg_queue_attributes.mq_maxmsg = MAX_MESSAGES;
    msg_queue_attributes.mq_flags = 0;
    msg_queue_attributes.mq_msgsize = sizeof(client_transfer_object_t);
    mqd_t client_queue = mq_open(queue_name, O_WRONLY, 0666, &msg_queue_attributes);
    if (client_queue == -1) {
        return 1;
    }
    if (send_data_to_client(tasks_list, client_queue) != 0) {
        write_log(task->task, "Failed task:");
        mq_close(client_queue);
        return 1;
    }
    write_log(task->task, "Finished task:");
    mq_close(client_queue);
    return 0;
}

// Sets up and runs task.
int run_task(tasks_list_t* tasks_list, pid_t pid, char*** envp) {
    pthread_mutex_lock(&list_access_mutex);
    if (tasks_list == NULL) {
        pthread_mutex_unlock(&list_access_mutex);
        return 1;
    }
    task_list_node_t* current_node = tasks_list->head;
    while (current_node != NULL) {
        if (current_node->task->pid == pid) {
            break;
        }
        current_node = current_node->next;
    }
    if (current_node == NULL) {
        pthread_mutex_unlock(&list_access_mutex);
        return 1;
    }
    write_log(current_node->task, "Starting task:");
    data_field_t* data_field = current_node->task->data_fields;
    switch (get_query_type(data_field->data)) {
        case ADD_TASK:
            if (add_task_query_handler(tasks_list, current_node, envp) != 0) {
                write_log(current_node->task, "Removed task:");
                destroy_node(tasks_list, current_node);
            }
            break;
        case LIST_TASKS:
            list_tasks_query_handler(tasks_list, current_node);
            write_log(current_node->task, "Removed task:");
            destroy_node(tasks_list, current_node);
            break;
        case REMOVE_TASK:
            remove_task_query_handler(tasks_list, current_node);
            write_log(current_node->task, "Removed task:");
            destroy_node(tasks_list, current_node);
            break;
        default:
            write_log(current_node->task, "Removed task:");
            destroy_node(tasks_list, current_node);
            pthread_mutex_unlock(&list_access_mutex);
            return 2;
    }
    pthread_mutex_unlock(&list_access_mutex);
    return 0;
}


////////////////////
// misc functions //
///////////////////


// Checks if given string is a flag.
int get_query_type(char* flag) {
    if (flag == NULL) {
        return 0;
    }
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

// Checks if date stored in string is ISO 8601 compliant. YYYY-MM-DDThh:mm:ss
int is_iso8601_date(char* string) {
    if (strlen(string) != 19) {
        return 0;
    }
    int counter = 0;
    while (*(string + counter) != '\0') {
        switch(counter){
            case 4:
            case 7:
                if (*(string + counter) != '-') {
                    return 0;
                }
                break;
            case 10:
                if (*(string + counter) != 'T') {
                    return 0;
                }
                break;
            case 13:
            case 16:
                if (*(string + counter) != ':') {
                    return 0;
                }
                break;
            default:
                if ((*(string + counter) < '0') || (*(string + counter) > '9')) {
                    return 0;
                }
                break;
        }
        ++counter;
    }
    return 1;
}

// Parses iso8601 date to seconds.
time_t parse_iso8601_date_to_seconds(char* string) {
    struct tm timeinfo = {0};

    if (sscanf(string, "%d-%d-%dT%d:%d:%d",
               &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
               &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec) != 6) {
        return -1;
    }
    timeinfo.tm_mon--;
    timeinfo.tm_year -= 1900;
    time_t seconds = mktime(&timeinfo);
    if (seconds == -1) {
        return -1;
    }

    return seconds;
}

// Converts time in form of string (seconds or timestamp) to seconds.
time_t get_time(char* time_string, int* time_type) {
    //timestamp: YYYY-MM-DDThh:mm:ss
    if (is_iso8601_date(time_string) == 1) {
        *time_type = 1;
        return parse_iso8601_date_to_seconds(time_string);
    }
    *time_type = 0;
    return convert_string_to_seconds(time_string);
}

//Converts given string  to seconds.
time_t convert_string_to_seconds(char* string) {
    time_t time = strtol(string, NULL, 10);
    if (time == 0 && (strcmp("0", string) != 0)){
        return -1;
    }
    return time;
}
