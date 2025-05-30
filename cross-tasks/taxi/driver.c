#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include "define.h"

static volatile int     running = 1;
int                     is_busy = 0;
int                     task_timer = 0;
time_t                  task_end_time = 0;
time_t                  last_heartbeat = 0;
int                     fd_read = -1;
int                     fd_write = -1;
pid_t                   pid = -1;

/* SIGINT handle */
void handle_signal(int sig);

/* Main process loop */
void process_loop();

/* Process command */
void process_command(char* command);

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    pid = getpid();

    // Create FIFO names with PID
    char fifo_cli_to_driver[PATH_LEN] = {0};
    char fifo_driver_to_cli[PATH_LEN] = {0};
    
    snprintf(fifo_cli_to_driver, sizeof(fifo_cli_to_driver), FIFO_CLI_TO_DRIVER, pid);
    snprintf(fifo_driver_to_cli, sizeof(fifo_driver_to_cli), FIFO_DRIVER_TO_CLI, pid);

    // Create FIFOs
    if (mkfifo(fifo_cli_to_driver, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo cli_to_driver failed");
        exit(EXIT_FAILURE);
    }   
    if (mkfifo(fifo_driver_to_cli, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo driver_to_cli failed");
        unlink(fifo_cli_to_driver);
        exit(EXIT_FAILURE);
    }

    if ((fd_read = open(fifo_cli_to_driver, O_RDONLY | O_NONBLOCK)) == -1) {
        perror("open read failed");
        unlink(fifo_cli_to_driver);
        unlink(fifo_driver_to_cli);
        exit(EXIT_FAILURE);
    }

    if ((fd_write = open(fifo_driver_to_cli, O_WRONLY)) == -1) {
        perror("open write failed");
        close(fd_read);
        unlink(fifo_cli_to_driver);
        unlink(fifo_driver_to_cli);
        exit(EXIT_FAILURE);
    }

    process_loop();

    close(fd_read);
    close(fd_write);
    unlink(fifo_cli_to_driver);
    unlink(fifo_driver_to_cli);
    
    //printf("Driver %d: Shutting down\n", pid);
    exit(EXIT_SUCCESS);
}

void handle_signal(int sig) {
    running = 0;
}

void process_loop() {
    while (running) {
        // Check for commands from CLI
        char command[BUFFER_SIZE];
        ssize_t count = read(fd_read, command, BUFFER_SIZE - 1);

        if (count > 0) {
            command[count] = '\0';
            
            process_command(command);
        } 
        else if (count == -1 && errno != EAGAIN) {
            perror("read failed");
            break;
        }

        // Check task completion
        if (is_busy && time(NULL) >= task_end_time) {
            //printf("Driver %d: Task completed\n", pid);
            is_busy = 0;
            task_timer = 0;
            write(fd_write, TASK_COMPLETE, 13);
        }

        // Send heartbeat periodically
        if (time(NULL) - last_heartbeat > HEARTBEAT_INTERVAL) {
            write(fd_write, HEARTBEAT, 9);
            last_heartbeat = time(NULL);
        }      

        usleep(100000);
    }
}

void process_command(char* command) {
    if (strncmp(command, SEND_TASK, 9) == 0) {
        if (is_busy) {
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), BUSY " %d", task_timer);
            write(fd_write, response, strlen(response));
            //printf("Driver %d: Rejected task, already busy\n", pid);
        } 
        else {
            task_timer = atoi(command + 10);
            task_end_time = time(NULL) + task_timer;
            is_busy = 1;
            //printf("Driver %d: Accepted task for %d seconds\n", pid, task_timer);
            write(fd_write, TASK_ACCEPTED, 13);
        }
    } 

    else if (strcmp(command, GET_STATUS) == 0) {
        char response[BUFFER_SIZE];
        if (is_busy) {
            snprintf(response, sizeof(response),  STATUS " " BUSY " %d", task_timer);
        } 
        else {
            snprintf(response, sizeof(response), STATUS " " AVAILABLE);
        }
        write(fd_write, response, strlen(response));
    } 

    else if (strncmp(command, PING, 4) == 0) {
        write(fd_write, PONG, 4);
        last_heartbeat = time(NULL);
    } 

    else if (strncmp(command, TERMINATE, 9) == 0) {
        //printf("Driver %d: Received terminate command\n", pid);
        running = 0;
        return;
    }
}