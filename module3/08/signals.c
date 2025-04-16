#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>
#include <errno.h>

#define OUTPUT_FILE "output.txt"
#define PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define WAIT        -1
#define SIGNAL      1

int     semid = -1;

/* Write in STDOUT */
void write_stdout(const char *str);

/* Create semaphore */
int create_semaphore();

/* Switch semaphore*/
int sem_op(int op);

/* Child process */
int child_process(int pipefd, int cycles);

/* Parent process */
int parent_process(int pipefd, int cycles);

typedef union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} semun;


int main(int argc, char *argv[]) 
{
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

    /* Create a semaphore */
    if (create_semaphore()) {
        semctl(semid, 0, IPC_RMID);
        exit(EXIT_FAILURE);
    }
    
    /* Create child process*/
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        semctl(semid, 0, IPC_RMID);
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
        int result = parent_process(pipefd[0], cycles);
        close(pipefd[0]);

        wait(NULL);
        semctl(semid, 0, IPC_RMID);
        exit(result);     
    }
       
}

int sem_op(int op) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int create_semaphore() {
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | PERMISSIONS);
    if (semid == -1) {
        perror("semget failed");
        return EXIT_FAILURE;
    }

    semun arg;
    arg.val = SIGNAL; 
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int child_process(int pipefd, int cycles) {       
    srand(time(NULL) ^ getpid());
        
    for (int i = 0; i < cycles; i++) {

        /* LOCK SEMAPHORE */
        if (sem_op(WAIT)) {
            return EXIT_FAILURE;
        };
            
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
                    perror("write");
                    break;
                }
            }
            if (bytesRead < 0) {
                perror("read");
            }
            close(fd);
        }

        /* UNLOCK SEMAPHORE */
        if (sem_op(SIGNAL)) {
            return EXIT_FAILURE;
        };
            
        int new_num = rand();   
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

int parent_process(int pipefd, int cycles) {
              
    for (int i = 0; i < cycles; i++) {         
        
        int received;
        if (read(pipefd, &received, sizeof(received)) != sizeof(received)) {
            perror("pipe");
            return EXIT_FAILURE;
        }
        
        /* LOCK SEMAPHORE */
        if (sem_op(WAIT)) {
            return EXIT_FAILURE;
        };
  
        char msg[128];
        snprintf(msg, sizeof(msg), "\nParent process: Received number: %d\n", received);
        write_stdout(msg);
        
        int fd = open(
            OUTPUT_FILE, 
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
            snprintf(msg, sizeof(msg), "Parent process: Write number in file %s\n", OUTPUT_FILE);
            write_stdout(msg);
        }
        
        /* UNLOCK SEMAPHORE */
        if (sem_op(SIGNAL)) {
            return EXIT_FAILURE;
        };
        
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}

