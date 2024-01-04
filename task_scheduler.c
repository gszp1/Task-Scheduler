#include "task_scheduler.h"

int main() {
    mqd_t server_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_EXCL | O_RDWR, 0666, NULL);
}
