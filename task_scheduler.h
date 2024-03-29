#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

// Macros //
#define SERVER_QUEUE_NAME "/server_queue"
#define USER_QUEUE_NAME "/user_queue"
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
#include <signal.h>
#include <spawn.h>


/////////////////
// data types  //
////////////////


// Enumerate for types of queries.
typedef enum {
    ADD_TASK = 1, // -a
    LIST_TASKS = 2, // -ls
    REMOVE_TASK = 3 // -rm
} query_type_t;

// Enumerate for task status.
typedef enum {
    DISABLED = 0,
    ACTIVE = 1
} task_status_t;

// Structure for transfering data from server to client
typedef struct {
    char content[MAX_ARGUMENT_SIZE + 1];
    char last_record_entry;
} client_transfer_object_t;

// Structure for object transferred by queue.
typedef struct {
    pid_t pid;
    char content[MAX_ARGUMENT_SIZE + 1];
} transfer_object_t;

// Node containing data sent by user.
typedef struct data_field{
    char data[MAX_ARGUMENT_SIZE + 1];
    struct data_field* next_field;
} data_field_t;

// Structure with data needed to run a task.
typedef struct {
    pid_t pid; // pid of task giver.
    timer_t timer; // timer.
    int cyclic;
    unsigned long id; // id of task.
    unsigned long number_of_fields; // number of fields sent by task giver.
    task_status_t task_status; // task status timer
    data_field_t* data_fields; // fields sent by task giver (single linked list).
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
    unsigned long max_id;
} tasks_list_t;

// Data given to function run by timer.
typedef struct timer_function_data {
    tasks_list_t* tasks_list;
    task_list_node_t* task;
    char*** envp;
} timer_function_data_t;

// functions declarations //


////////////////////////////////////////
// linked list manipulation functions //
///////////////////////////////////////


// Initialize tasks linked list.
int task_list_init(tasks_list_t** tasks_list, int do_logs);

// Destroys task.
void destroy_task(task_t* task);

// removes node from tasks list and frees resources.
void destroy_node(tasks_list_t* tasks_list, task_list_node_t* node);

// Destroy tasks linked list.
void tasks_list_destroy(tasks_list_t* tasks_list);

// Creates new task.
task_t* create_new_task(char* field, pid_t pid); 

// Creates new data field.
data_field_t* create_data_field(char* data, pid_t pid);

// Adds data field read from queue to task.
int add_data_to_task(tasks_list_t* tasks_list, pid_t pid, data_field_t* data_field);

// Adds task to linked list.
int add_task(task_t* task, tasks_list_t* tasks_list);


///////////////////////////////
// task processing functions //
//////////////////////////////


// Send program arguments to server.
int queue_send_arguments(int argc, char* argv[], mqd_t message_queue);

// Sets up and runs task.
int run_task(tasks_list_t* tasks_list, pid_t pid, char*** envp);


////////////////////
// misc functions //
///////////////////

// Checks if given string is a flag.
int get_query_type(char* flag);

// Checks if date stored in string is ISO 8601 compliant. YYYY-MM-DDThh:mm:ss
int is_iso8601_date(char* string);

// Parses iso8601 date to seconds.
time_t parse_iso8601_date_to_seconds(char* string);

// Converts time in form of string (seconds or timestamp) to seconds.
time_t get_time(char* time_string, int* time_type);

//Converts given string  to seconds.
time_t convert_string_to_seconds(char* string);

#endif //TASK_SCHEDULER_H
