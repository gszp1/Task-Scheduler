#!/bin/bash

# Don't forget to add execute permission: sudo chmod +x compile.sh

gcc task_scheduler.c task_scheduler.h -lrt -o task_scheduler_out