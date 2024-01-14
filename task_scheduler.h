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

// data types  //

typedef struct {
    task* head;
    task* tail;
    unsigned long max_id;
} tasks_list;

typedef struct {
    task* next;
    task* prev;
    task* task;
} task_list_node;

typedef struct {
    unsigned long id;
} task;

// functions declarations //

#endif //TASK_SCHEDULER_TASK_SCHEDULER_H
