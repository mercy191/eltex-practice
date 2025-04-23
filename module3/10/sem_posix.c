#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#define OUTPUT_FILE "output.txt"
#define SEM_NAME    "/my_named_semaphore"
#define PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

sem_t*  sem;

/* Write in STDOUT */
void write_stdout(const char *str);

 /* Create a semaphore */
int create_semaphore();

/* Child process */
int child_process(int pipefd, int cycles);

/* Parent process */
int parent_process(int pipefd, int cycles);


int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        fprintf(stderr, "Error input arguments, usage: %s <cycles_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int cycles = atoi(argv[1]);
    if (cycles <= 0) {
        fprintf(stderr, "Negative cycles number: %d\n", cycles);
        exit(EXIT_FAILURE);
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    /* Create a semaphore */
    if (create_semaphore() == -1) {
        cleanup();
        exit(EXIT_FAILURE);
    }

    /* Create child process*/
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        cleanup();
        exit(EXIT_FAILURE);
    }
    /* Child process */
    else if (pid == 0) {
        close(pipefd[0]);
        int result = child_process(pipefd[1], cycles);
        close(pipefd[1]);

        sem_close(sem);
        exit(result);
    }
    /* Parent process */
    else {
        close(pipefd[1]);
        int result = parent_process(pipefd[0], cycles);
        close(pipefd[0]);

        wait(NULL);
        cleanup();
        exit(result);     
    }    
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

void cleanup() {
    if (sem != NULL && sem_close(sem) == -1) {
        perror("sem_close failed");
    }

    if (getpid() == 1) {  
        if (sem_unlink(SEM_NAME) == -1) {
            perror("sem_unlink failed");
        }
    }
}

int create_semaphore() {
    sem = sem_open(SEM_NAME, O_CREAT, PERMISSIONS, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return -1;
    }

    return 0;
}

int child_process(int pipefd, int cycles) {       
    srand(time(NULL) ^ getpid());
        
    for (int i = 0; i < cycles; i++) {

        /* LOCK SEMAPHORE */
        if (sem_wait(sem) == -1) {
            perror("sem_wait failed");
            return EXIT_FAILURE;
        }
            
        char msg[128];
        int fd = open(OUTPUT_FILE, O_RDONLY);
        if (fd < 0) {
            snprintf(msg, sizeof(msg), "\nChild process: File %s doesn't exists or empty.\n", OUTPUT_FILE);
            write_stdout(msg);
        } 
        else {                             
            snprintf(msg, sizeof(msg), "\nChild process: Read file %s:\n", OUTPUT_FILE);
            write_stdout(msg);

            char buffer[256];
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

        /* UNLOCK SEMAPHORE */
        if (sem_post(sem) == -1) {
            perror("sem_post failed");
            return EXIT_FAILURE;
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

int parent_process(int pipefd, int cycles) {
              
    for (int i = 0; i < cycles; i++) {         
        
        int received;
        if (read(pipefd, &received, sizeof(received)) != sizeof(received)) {
            perror("pipe failed");
            return EXIT_FAILURE;
        }
        
        /* LOCK SEMAPHORE */
        if (sem_wait(sem) == -1) {
            perror("sem_wait failed");
            return EXIT_FAILURE;
        }
  
        char msg[128];
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
        
        /* UNLOCK SEMAPHORE */
        if (sem_post(sem) == -1) {
            perror("sem_post failed");
            return EXIT_FAILURE;
        }
        
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}
