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

volatile sig_atomic_t file_locked = 0;

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
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    /* Create child process*/
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    /* Child process */
    else if (pid == 0) {
        close(pipefd[0]);

        if (child_process(pipefd[1], cycles) == EXIT_SUCCESS) {
            close(pipefd[1]);
            exit(EXIT_SUCCESS);
        }
        else {
            close(pipefd[1]);
            exit(EXIT_FAILURE);
        }
    }
    /* Parent process */
    else {
        close(pipefd[1]);

        if (parent_process(pid, pipefd[0], cycles) == EXIT_SUCCESS) {
            close(pipefd[0]);
            exit(EXIT_SUCCESS);
        }
        else {
            close(pipefd[0]);
            exit(EXIT_SUCCESS);
        }       
    }
       
}

/* SIGUSR1 handle */
void handle_sigusr1(int sig) {
    file_locked = 1;
}

/* SIGUSER2 handle */
void handle_sigusr2(int sig) {
    file_locked = 0;
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int child_process(int pipefd, int cycles) {       
    srand(time(NULL) ^ getpid());
        
    for (int i = 0; i < cycles; i++) {
        /* SIGGUSR1 */
        while (file_locked) {
            usleep(10000); 
        }
            
        int fd = open("output.txt", O_RDONLY);
        if (fd < 0) {
            write_stdout("\nChild process: File 'output.txt' doesn't exists or empty.\n");
        } 
        else {                             
            char buffer[256];
            ssize_t bytesRead;
            write_stdout("\nChild process: Read file 'output.txt':\n");
            while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
                if (write(STDOUT_FILENO, buffer, bytesRead) != bytesRead) {
                    perror("write");
                    break;
                }
            }
            if (bytesRead < 0) {
                perror("read");
            }
            close(fd);
        }
            
        int new_num = rand();
        char msg[128];
        snprintf(msg, sizeof(msg), "Child process: Send new number: %d\n", new_num);
        write_stdout(msg);

        if (write(pipefd, &new_num, sizeof(new_num)) != sizeof(new_num)) {
            perror("pipe");
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
            perror("pipe");
            return EXIT_FAILURE;
        }
        
        /* Send SIGUSR1 */
        if (kill(child_pid, SIGUSR1) == -1) {
            perror("kill");
        }
  
        char msg[128];
        snprintf(msg, sizeof(msg), "\nParent process: Received number: %d\n", received);
        write_stdout(msg);
        
        int fd = open(
            "output.txt", 
            O_WRONLY | O_CREAT | O_APPEND, 
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0) {
            perror("open");
        } 
        else {
            snprintf(msg, sizeof(msg), "  Parent process: Number: %d\n", received);
            if (write(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) {
                perror("write");
            }
            close(fd);
            write_stdout("Parent process: Write number in file 'output.txt'\n");
        }
        
        /* Send SIGUSR2 */
        if (kill(child_pid, SIGUSR2) == -1) {
            perror("kill");
        }
        
        sleep(1);
    }
    
    wait(NULL);
    return EXIT_SUCCESS;
}

