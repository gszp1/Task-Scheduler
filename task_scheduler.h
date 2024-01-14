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

// data types  //

typedef struct {
    pid_t pid;
    char content[MAX_ARGUMENT_SIZE + 1];
} transfer_object_t;

typedef struct {
    unsigned long id;
    pid_t pid;
    timer_t timer;
} task_t;

typedef struct task_list_node {
    struct task_list_node* next;
    struct task_list_node* prev;
    task_t* task;
} task_list_node_t;

typedef struct {
    task_list_node_t* head;
    task_list_node_t* tail;
    unsigned long max_id;
    pthread_mutex_t list_access_mutex;
} tasks_list_t;

// functions declarations //

int task_list_init(tasks_list_t* tasks_list);

void tasks_list_destroy(tasks_list_t* tasks_list);

#endif //TASK_SCHEDULER_TASK_SCHEDULER_H
