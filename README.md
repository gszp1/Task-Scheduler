# Task-Scheduler
Celem projektu jest napisanie programu do harmonogramowania zadań
będącego funkcjonalnym odpowiednikiem programu cron
(https://pl.wikipedia.org/wiki/Cron) znanego z systemów unixowych. Program powinien spełniać następujące założenia:

powinien być oparty na mechanizmie zegarów interwałowych;
sterowanie pracą programu powinno odbywać przez argumenty wiersza poleceń;
powinno być możliwe uruchomienie tylko jednej instancji programu (pierwsze uruchomienie pliku wykonywalnego uruchamia serwer usługi, kolejne są klientami);
kolejne uruchomienia pliku wykonywalnego z odpowiednimi argumentami służyć mają sterowaniu pracą programu;
komunikacja klient-serwer ma przebiegać z wykorzystaniem mechanizmu komunikatów;
powinno być możliwe planowanie uruchomienia wskazanego programu poprzez podanie czasu względnego, bezwzględnego lub cyklicznie;
musi istnieć możliwość planowanego uruchomienia programu z argumentami wiersza poleceń;
program musi mieć możliwość wyświetlenia listy zaplanowanych zadań;
musi istnieć możliwość anulowania wybranego zadania;
program powinien mieć możliwość zapisywania logów z wykorzystaniem biblioteki zrealizowane w ramach projektu 1;
zakończenie pracy programu powinno być równoznaczne z anulowaniem wszystkich zaplanowanych zadań;
aplikacja powinna być zgodna ze standardami POSIX i języka C.

0. Program specifications:  
-  Program commands are inserted through command line.  
-  Inserting more flags than required or not providing all required ones, will cause the scheduler to ignore given task, even if it is partially correct.  
-  Provide all required arguments, no more, no less.
-  Transfering data between processes is based on POSIX queues, namely through mq_queues. The main, server, queue is named /serer_queue.  
-  Queue created by user, through which server sends all tasks in the list is named /user_queuepid, where pid is replaced with user's process pid.  
-  In both cases, data is transfered in 256 bytes blocks, with the last one being empty. 
1. Add task command.  
-  Adds a task to run other program as child process.  
-  add task structure: ./task_scheduler_out -a <seconds/timestamp> <repeat_time> <program_name> <args>  
-  relative time is given by seconds, absolute time with timestamp.
2. List tasks command.  
-  Lists all tasks held on server.
-  list all tasks command structure: ./task_scheduler_out -ls
-  wrong command example: ./task_scheduler_out -ls 2137
3. Remove task command.  
-  remove task structure: ./task_scheduler_out -rm <task_id>  
-  correct command example: ./task_scheduler_out -rm 2137  
-  wrong command example: ./task_scheduler_out -rm 214454 12232 hello
4. Prepared scripts
-  Repository includes few scripts that are not needed to run scheduler, but make development process and runing easier and faster.
-  These scripts require bash installed, so they may not work properly outside Linux distributions (not tested).
-  Add permission to execute these scripts.
-  run.sh - runs scheduler.
-  compile.sh - runs command to compile scheduler source code.
-  compile_and_run.sh - compiles and runs scheduler.
-  set_permissions.sh - sets permissions of all other scripts to execute.
-  rm_queue.sh - removes server queue from system.
5. Logs
-  You can turn on making logs during program execution, by adding do_logs argument in command line when running server.
-  Logs will be saved to logs.txt
-  By default they are turned off
