cmake_minimum_required(VERSION 3.27)
project(Task_Scheduler C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Task_Scheduler
        task_scheduler.c
        task_scheduler.h)
