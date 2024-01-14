#ifndef TASK_SCHEDULER_TASK_SCHEDULER_H
#define TASK_SCHEDULER_TASK_SCHEDULER_H

// Macros //
#define SERVER_QUEUE_NAME "/server_queue"
#define MAX_MESSAGES 10

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

// data types  //

typedef struct {
    task_t* head;
    task_t* tail;
    unsigned long max_id;
} tasks_list_t;

typedef struct {
    task_t* next;
    task_t* prev;
    task_t* task;
} task_list_node_t;

typedef struct {
    pid_t pid;
    unsigned long id;
} task_t;

// functions declarations //

int task_list_init(tasks_list_t* tasks_list);

#endif //TASK_SCHEDULER_TASK_SCHEDULER_H
