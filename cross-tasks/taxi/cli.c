#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <glob.h>
#include "define.h"

typedef struct {
    pid_t pid;
    int fd_write; 
    int fd_read;  
    int is_busy;
    int task_timer;
    time_t last_activity;
} Driver;

static volatile int     running = 1;
Driver                  drivers[MAX_DRIVERS];
size_t                  num_drivers = 0;
int                     epoll_fd;

/* SIGINT handle */
void handle_signal(int sig);

/* Add fd in epoll events */
int add_to_epoll(int fd);

/* Main process loop */
void process_loop(); 

/* Process command */
void process_command(char* command);

/* Create new driver process*/
int create_driver();

/* Send task for driver */
int send_task(pid_t pid, int task_timer);

/* Get driver status */
int get_status(pid_t pid);

/* Terminate driver process */
int terminate_driver(pid_t pid);

/* Ping driver if he not active for a while */
int ping_driver(int index);

/* Clear information about driver */
void cleanup_driver(int index);

/* Check driver activities */
void drivers_activity();

/* Processing responce from driver */
void driver_response(int fd);


int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        exit(EXIT_FAILURE);
    }

    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    if (add_to_epoll(STDIN_FILENO) == -1) {
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("=== Taxi Management System ===\n");
    printf("Commands:\n");
    printf("\t" CREATE_DRIVER " - start new driver process\n");
    printf("\t" SEND_TASK " <pid> <seconds> - assign task to driver\n");
    printf("\t" GET_STATUS " <pid> - get driver status\n");
    printf("\t" GET_DRIVERS " - list all available drivers\n");
    printf("\t" TERMINATE " <pid> - terminate driver\n");
    printf("\t" EXIT " - exit CLI\n\n");

    process_loop();

    for (int i = num_drivers - 1; i >= 0; i--) {
        terminate_driver(drivers[i].pid);
    }
    
    close(epoll_fd);
    printf("CLI shutdown complete\n");
    exit(EXIT_SUCCESS);
}

void handle_signal(int sig) {
    running = 0;
}

int add_to_epoll(int fd) {
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("epoll_ctl failed");
        return -1;
    }

    return 0;
}

void process_loop() {
    struct epoll_event events[MAX_DRIVERS + 1];
    while (running) {
        printf("> ");
        fflush(stdout);
     
        int nfds = epoll_wait(epoll_fd, events, MAX_DRIVERS + 1, -1);      
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                char command[BUFFER_SIZE];
                ssize_t count = read(STDIN_FILENO, command, BUFFER_SIZE - 1);
                
                if (count > 0) {
                    command[count] = '\0';
                    command[strcspn(command, "\n")] = '\0';
                    process_command(command);
                }
                else if (count == -1 && errno != EAGAIN) {
                    perror("read failed");
                    running = 0;
                    break;
                }
            } 
            else {
                driver_response(events[i].data.fd);
            }
        }

        drivers_activity();
    }
}

void process_command(char* command) {
    char *token = strtok(command, " ");
    
    if (token == NULL) return;
    
    if (strcmp(token, CREATE_DRIVER) == 0) {
        int ret = create_driver();
        if (ret == 0) {
            printf("Maximum count of drivers reached (%d)\n", MAX_DRIVERS);
            return;
        }
        else if (ret == -1) {
            printf("Driver not created\n");
        }
        else {
            printf("Driver created with PID: %d\n", ret);
        }         
    } 

    else if (strcmp(token, SEND_TASK) == 0) {
        char *pid_str = strtok(NULL, " ");
        char *timer_str = strtok(NULL, " ");
        
        if (pid_str && timer_str) {
            pid_t pid = atoi(pid_str);
            int timer = atoi(timer_str);
            
            int ret = send_task(pid, timer);
            if (ret == 0) {
                printf("Driver with PID %d not found\n", pid);
            }
            else if (ret == -1) {
                printf("Failed to send task to driver %d\n", pid);
            }
            else {
                printf("Task sent to driver %d for %d seconds\n", pid, timer);
            }              
        } 
        else {
            printf("Usage: " SEND_TASK " <pid> <seconds>\n");
        }
    } 
    
    else if (strcmp(token, GET_STATUS) == 0) {
        char *pid_str = strtok(NULL, " ");
        if (pid_str) {
            pid_t pid = atoi(pid_str);
                          
            int ret = get_status(pid);
            if (ret == 0) {
                printf("Driver with PID %d not found\n", pid);
            }
            else if (ret == -1) {
                printf("Failed to get status from driver %d\n", pid);
            }
            else {
                printf("Status request sent to driver %d\n", pid);
            }  
        } 
        else {
            printf("Usage: " GET_STATUS " <pid>\n");
        }
    } 

    else if (strcmp(token, GET_DRIVERS) == 0) {
        printf("Active drivers (%ld):\n", num_drivers);
        for (int i = 0; i < num_drivers; i++) {
            printf("PID: %d - ", drivers[i].pid);
            if (drivers[i].is_busy) {
                printf("Busy (task time remaining: %d sec)\n", drivers[i].task_timer);
            } else {
                printf("Available\n");
            }
        }
    } 

    else if (strcmp(token, TERMINATE) == 0) {
        char *pid_str = strtok(NULL, " ");
        if (pid_str) {
            pid_t pid = atoi(pid_str);
            
            int ret = terminate_driver(pid);
            if (ret == 0) {
                printf("Driver with PID %d not found\n", pid);
            }
            else if (ret == -1) {
                printf("Failed to send terminate to driver %d\n", pid);
            }
            else {
                printf("Termination signal sent to driver %d\n", pid);
            } 
        } 
        else {
            printf("Usage: " TERMINATE " <pid>\n");
        }
    } 

    else if (strcmp(token, EXIT) == 0) {
        printf("Exit...\n");
        running = 0;
    } 

    else {
        printf("Unknown command.\n");
    }

    return;
}

int create_driver() {
    if (num_drivers >= MAX_DRIVERS) {       
        return 0;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return -1;
    }
    else if (pid == 0) {
        execl("./driver", "driver", NULL);
        perror("exec failed");
        return -1;
    } 
    else {
        // Parent process - setup communication
        char fifo_cli_to_driver[PATH_LEN];
        char fifo_driver_to_cli[PATH_LEN];
        
        snprintf(fifo_cli_to_driver, sizeof(fifo_cli_to_driver), FIFO_CLI_TO_DRIVER, pid);
        snprintf(fifo_driver_to_cli, sizeof(fifo_driver_to_cli), FIFO_DRIVER_TO_CLI, pid);

        // Wait for driver to create FIFOs
        int attempts = 0;
        while (access(fifo_cli_to_driver, F_OK) == -1 || access(fifo_driver_to_cli, F_OK) == -1) {
            if (++attempts > 10) {
                perror("access failed");
                kill(pid, SIGTERM);
                return -1;
            }
            usleep(100000);
        }

        int fd_write = open(fifo_cli_to_driver, O_WRONLY);  
        if (fd_write == -1) {
            perror("open cli_to_driver failed");
            kill(pid, SIGTERM);
            return -1;
        }

        int fd_read = open(fifo_driver_to_cli, O_RDONLY | O_NONBLOCK);  
        if (fd_read == -1) {
            perror("open driver_to_cli failed");
            close(fd_write);
            kill(pid, SIGTERM);
            return -1;
        }

        drivers[num_drivers].pid = pid;
        drivers[num_drivers].fd_write = fd_write;
        drivers[num_drivers].fd_read = fd_read;
        drivers[num_drivers].is_busy = 0;
        drivers[num_drivers].task_timer = 0;
        drivers[num_drivers].last_activity = time(NULL);
        
        add_to_epoll(fd_read);
        num_drivers++;
        
        ping_driver(num_drivers - 1);

        return pid;
    }
}

int send_task(pid_t pid, int task_timer) {
    for (int i = 0; i < num_drivers; i++) {
        if (drivers[i].pid == pid) {
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), SEND_TASK " %d", task_timer);
            
            if (write(drivers[i].fd_write, command, strlen(command)) == -1) {
                perror("write failed");           
                cleanup_driver(i);
                return -1;
            }
            
            drivers[i].last_activity = time(NULL);
            return pid;
        }
    }
    
    return 0;
}

int get_status(pid_t pid) {
    for (int i = 0; i < num_drivers; i++) {
        if (drivers[i].pid == pid) {
            if (write(drivers[i].fd_write, GET_STATUS, 10) == -1) {
                perror("write failed");
                cleanup_driver(i);
                return -1;
            }
            
            drivers[i].last_activity = time(NULL);
            return pid;
        }
    }
    return 0;
}

int terminate_driver(pid_t pid) {
    for (int i = 0; i < num_drivers; i++) {
        if (drivers[i].pid == pid) {
            if (write(drivers[i].fd_write, TERMINATE, 9) == -1) {
                perror("write failed");
                return -1;
            }
            
            cleanup_driver(i);
            return pid;
        }
    }

    return 0;
}

void cleanup_driver(int index) {  
    // Close FDs
    close(drivers[index].fd_read);
    close(drivers[index].fd_write);
    
    // Remove from epoll
    struct epoll_event event;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, drivers[index].fd_read, &event);
    
    // Remove FIFOs
    char fifo_cli_to_driver[PATH_LEN];
    char fifo_driver_to_cli[PATH_LEN];
    
    snprintf(fifo_cli_to_driver, sizeof(fifo_cli_to_driver), FIFO_CLI_TO_DRIVER, drivers[index].pid);
    snprintf(fifo_driver_to_cli, sizeof(fifo_driver_to_cli), FIFO_DRIVER_TO_CLI, drivers[index].pid);
    
    unlink(fifo_cli_to_driver);
    unlink(fifo_driver_to_cli);
    
    // Shift array
    for (int i = index; i < num_drivers - 1; i++) {
        drivers[i] = drivers[i + 1];
    }
    memset(&drivers[num_drivers - 1], 0, sizeof(Driver));
    
    num_drivers--;
}

int ping_driver(int index) {
    if (write(drivers[index].fd_write, PING, 4) == -1) {
        perror("write failed");
        return -1;      
    } 

    return 0;
}

void driver_response(int fd) {
    char buffer[BUFFER_SIZE];
    ssize_t count = read(fd, buffer, BUFFER_SIZE - 1);
    
    if (count > 0) {
        buffer[count] = '\0';
        
        for (int i = 0; i < num_drivers; i++) {
            if (drivers[i].fd_read == fd) {
                drivers[i].last_activity = time(NULL);
                
                if (strncmp(buffer, BUSY, 4) == 0) {
                    drivers[i].is_busy = 1;
                    drivers[i].task_timer = atoi(buffer + 5);
                    printf("Driver %d is busy with %d seconds remaining\n", drivers[i].pid, drivers[i].task_timer);
                } 
                else if (strncmp(buffer, STATUS, 6) == 0) {
                    printf("Driver %d status: %s\n", drivers[i].pid, buffer + 7);
                } 
                else if (strcmp(buffer, TASK_ACCEPTED) == 0) {
                    printf("Driver %d accepted the task\n", drivers[i].pid);
                } 
                else if (strcmp(buffer, TASK_COMPLETE) == 0) {
                    drivers[i].is_busy = 0;
                    drivers[i].task_timer = 0;
                    printf("Driver %d completed its task\n", drivers[i].pid);
                } 
                else if (strcmp(buffer, PONG) == 0) {
                    drivers[i].last_activity = time(NULL);
                    printf("Driver %d is active\n", drivers[i].pid);
                } 
                else if (strcmp(buffer, HEARTBEAT) == 0) {
                    write(drivers[i].fd_write, PONG, 4);
                } 
                else {
                    printf("Driver %d: Unknown response: %s\n", drivers[i].pid, buffer);
                }
                break;
            }
        }
    } 
    else if (count == -1 && errno != EAGAIN) {
        perror("read failed");
        for (int i = 0; i < num_drivers; i++) {
            if (drivers[i].fd_read == fd) {
                printf("Driver %d connection error, cleaning up\n", drivers[i].pid);
                cleanup_driver(i);
                break;
            }
        }
    }
}

void drivers_activity() {
    time_t now = time(NULL);
    
    for (int i = num_drivers - 1; i >= 0; i--) {

        if (now - drivers[i].last_activity > DRIVER_TIMEOUT * 2) {
            printf("Driver %d not responding, terminating\n", drivers[i].pid);
            terminate_driver(drivers[i].pid);
        }
        else if (now - drivers[i].last_activity > DRIVER_TIMEOUT) {
            printf("Driver %d inactive for too long, pinging\n", drivers[i].pid);
            if (ping_driver(i) == -1) {
                printf("Failed to ping driver %d\n", drivers[i].pid);
            }
        }          
    }
}
