#ifndef TASK_SCHEDULER_TASK_SCHEDULER_H
#define TASK_SCHEDULER_TASK_SCHEDULER_H

// Macros //
#define SERVER_QUEUE_NAME "/server_queue"
#define MAX_MESSAGES 10
#define MAX_ARGUMENT_SIZE 255

// Includes //
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

// data types  //

// Enumerate for types of queries.
typedef enum {
    ADD_TASK = 1,
    LIST_TASKS = 2,
    REMOVE_TASK = 3
} query_type_t;

// Structure for object transferred by queue.
typedef struct {
    pid_t pid;
    char content[MAX_ARGUMENT_SIZE + 1];
} transfer_object_t;

// Structure with data needed to run a task.
typedef struct {
    unsigned long id;
    pid_t pid;
    timer_t timer;
} task_t;

// Tasks linked list node.
typedef struct task_list_node {
    struct task_list_node* next;
    struct task_list_node* prev;
    task_t* task;
} task_list_node_t;

// Tasks linked list.
typedef struct {
    task_list_node_t* head;
    task_list_node_t* tail;
    pthread_mutex_t list_access_mutex;
    unsigned long max_id;
} tasks_list_t;

// functions declarations //

// Initialize tasks linked list.
int task_list_init(tasks_list_t* tasks_list);

// Destroy tasks linked list.
void tasks_list_destroy(tasks_list_t* tasks_list);

// Send program arguments to server.
int queue_send_arguments(int argc, char* argv[], mqd_t message_queue);

// Checks if given string is a flag.
int get_query_type(char* flag);

// Removes task with given ID.
int remove_task_by_id(tasks_list_t* tasks_list, unsigned long id);

#endif //TASK_SCHEDULER_TASK_SCHEDULER_H
