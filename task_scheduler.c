#include "task_scheduler.h"

// main function //

int main(int argc, char* argv[], char* envp[]) {
    // main server queue attributes
    struct mq_attr server_msg_queue_attributes;
    server_msg_queue_attributes.mq_maxmsg = MAX_MESSAGES;
    server_msg_queue_attributes.mq_flags = 0;
    server_msg_queue_attributes.mq_msgsize = sizeof(transfer_object_t);

    // try to create main queue
    mqd_t server_msg_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_EXCL | O_RDONLY, 0666, &server_msg_queue_attributes);
    if ((server_msg_queue == -1) && (errno == EEXIST)) {
        printf("Running process as client.\n");

        server_msg_queue = mq_open(SERVER_QUEUE_NAME, O_WRONLY, 0666, &server_msg_queue_attributes);
        if (server_msg_queue == -1) {
            printf("Failed to connect with server queue.\n");
            return 1;
        }

        // if user entered command for displaying list of tasks
        int is_list_tasks_query = 0;
        if (argc > 1) {
            is_list_tasks_query = (get_query_type(argv[1]) == LIST_TASKS) && (argc <= 2);
        }
        mqd_t user_queue;
        char user_queue_name[32];

        if (is_list_tasks_query) {
            sprintf(user_queue_name, "%s%d", USER_QUEUE_NAME, getpid());
            struct mq_attr client_queue_attributes;
            client_queue_attributes.mq_maxmsg = 10;
            client_queue_attributes.mq_flags = 0;
            client_queue_attributes.mq_msgsize = sizeof(client_transfer_object_t);
            user_queue = mq_open(user_queue_name, O_CREAT | O_EXCL | O_RDONLY, 0666,  &client_queue_attributes);
            if (user_queue == -1) {
                mq_close(server_msg_queue);
                printf("Failed to open client queue.\n");
                return 1;
            }
        }

        if (queue_send_arguments(argc, argv, server_msg_queue)) {
            printf("Failed to send arguments.\n");
            mq_close(server_msg_queue);
            if (is_list_tasks_query) {
                mq_close(user_queue);
                mq_unlink(user_queue_name);
            }
            return 2;
        }

        if (is_list_tasks_query) {
            char reading_finished = 0;
            char first_field = 1;
            while (reading_finished == 0) {
                client_transfer_object_t transfer_object;
                mq_receive(user_queue, (char*)(&transfer_object), sizeof(client_transfer_object_t), 0);
                if (transfer_object.content[0] == '\0') {
                    printf("\n");
                    reading_finished = 1;
                    continue;
                }
                printf("%s ", transfer_object.content);
                if (first_field == 1) {
                    printf("| ");
                    first_field = 0;
                }
                if(transfer_object.last_record_entry == 1) {
                    printf("\n");
                    first_field = 1;
                }
            }
            mq_close(user_queue);
            mq_unlink(user_queue_name);
        }

        mq_close(server_msg_queue);
    } else {
        printf("Running process as server.\n");

        tasks_list_t* tasks_list = NULL;
        if (task_list_init(&tasks_list)) {
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
                task_t* new_task = create_new_task(transfer_object.content, transfer_object.pid);
                add_task(new_task, tasks_list);
            } else {
                if (strcmp("", transfer_object.content) == 0) {
                    run_task(tasks_list, transfer_object.pid, &envp);
                } else {
                    data_field_t* data_field = create_data_field(transfer_object.content, transfer_object.pid);
                    add_data_to_task(tasks_list, transfer_object.pid, data_field);
                }
            }
        }
        tasks_list_destroy(tasks_list);
        mq_close(server_msg_queue);
        mq_unlink(SERVER_QUEUE_NAME);
    }
    return 0;
}