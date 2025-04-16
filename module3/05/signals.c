#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define OUTPUT_FILE "output.txt"
#define SLEEP_TIME  10000
#define MSG_BUF     128

#define LOCK        1
#define UNLOCK      0

volatile sig_atomic_t file_locked = UNLOCK;

/* SIGUSR1 handle */
void handle_sigusr1(int sig);

/* SIGUSER2 handle */
void handle_sigusr2(int sig);

/* Write in STDOUT */
void write_stdout(const char *str);

/* Child process */
int child_process(int pipefd, int cycles);

/* Parent process */
int parent_process(int child_pid, int pipefd, int cycles);


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error input arguments, usage: %s <cycles_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int cycles = atoi(argv[1]);
    if (cycles <= 0) {
        fprintf(stderr, "Enter a positive number.\n");
        exit(EXIT_FAILURE);
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }
    
    /* Create child process*/
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    /* Child process */
    else if (pid == 0) {
        close(pipefd[0]);
        int result = child_process(pipefd[1], cycles);
        close(pipefd[1]);
        
        exit(result);
    }
    /* Parent process */
    else {
        close(pipefd[1]);
        int result = parent_process(pid, pipefd[0], cycles);
        close(pipefd[0]);
        
        exit(result);     
    }
       
}

/* SIGUSR1 handle */
void handle_sigusr1(int sig) {
    file_locked = LOCK;
}

/* SIGUSER2 handle */
void handle_sigusr2(int sig) {
    file_locked = UNLOCK;
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int child_process(int pipefd, int cycles) {       
    srand(time(NULL) ^ getpid());
        
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    for (int i = 0; i < cycles; i++) {
        /* Wait SIGGUSR1 */
        while (file_locked) {
            usleep(SLEEP_TIME); 
        }
          
        char msg[MSG_BUF];
        int fd = open(OUTPUT_FILE, O_RDONLY);
        if (fd < 0) {
            snprintf(msg, sizeof(msg), "\nChild process: File %s doesn't exists or empty.\n", OUTPUT_FILE);
            write_stdout(msg);
        } 
        else {                             
            snprintf(msg, sizeof(msg), "\nChild process: Read file %s:\n", OUTPUT_FILE);
            write_stdout(msg);

            char buffer[MSG_BUF];
            ssize_t bytesRead;
            while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
                if (write(STDOUT_FILENO, buffer, bytesRead) != bytesRead) {
                    perror("write failed");
                    break;
                }
            }
            if (bytesRead < 0) {
                perror("read failed");
            }
            close(fd);
        }
            
        int new_num = rand();
        
        snprintf(msg, sizeof(msg), "Child process: Send new number: %d\n", new_num);
        write_stdout(msg);

        if (write(pipefd, &new_num, sizeof(new_num)) != sizeof(new_num)) {
            perror("pipe failed");
            return EXIT_FAILURE;           
        }

        sleep(1);
    }
        
    return EXIT_SUCCESS;   
}

int parent_process(int child_pid, int pipefd, int cycles) {
              
    for (int i = 0; i < cycles; i++) {         
        
        int received;
        if (read(pipefd, &received, sizeof(received)) != sizeof(received)) {
            perror("pipe failed");
            return EXIT_FAILURE;
        }
        
        /* Send SIGUSR1 */
        if (kill(child_pid, SIGUSR1) == -1) {
            perror("kill failed");
        }
  
        char msg[MSG_BUF];
        snprintf(msg, sizeof(msg), "\nParent process: Received number: %d\n", received);
        write_stdout(msg);
        
        int fd = open(
            OUTPUT_FILE, 
            O_WRONLY | O_CREAT | O_APPEND, 
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0) {
            perror("open failed");
        } 
        else {
            snprintf(msg, sizeof(msg), "  Parent process: Number: %d\n", received);
            if (write(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) {
                perror("write failed");
            }
            close(fd);
            snprintf(msg, sizeof(msg), "Parent process: Write number in file %s\n", OUTPUT_FILE);
            write_stdout(msg);
        }
        
        /* Send SIGUSR2 */
        if (kill(child_pid, SIGUSR2) == -1) {
            perror("kill failed");
        }
        
        sleep(1);
    }
    
    wait(NULL);
    return EXIT_SUCCESS;
}

