#!/bin/bash

# Don't forget to add execute permission: sudo chmod +x compile.sh

gcc task_scheduler.c task_scheduler.h task_scheduler_defs.c app_state_logger.h app_state_logger.c -lrt -o task_scheduler_out
