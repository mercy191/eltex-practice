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
#include <sys/shm.h>
#include <time.h>

#define OUTPUT_FILE "output.txt"
#define PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define CHLD_COUNT  2

#define WAIT        -1
#define SIGNAL      1
#define SEM_COUNT   2
#define READER      0
#define WRITER      1

unsigned int*   read_count;
int             shmid = -1;
int             semid = -1;

/* Write in STDOUT */
void write_stdout(const char *str);

/* Clean resources */
void cleanup();

/* Create shared memory */  
int create_shared_memory();

/* Create semaphore */
int create_semaphore();

/* Switch semaphore*/
int sem_op(int semnum, int op);

/* Lock semaphore */
int lock_semaphore();

/* Unlock semaphore */
int unlock_semaphore();

/* Child process */
int child_process(int semid, int cycles);

/* Parent process */
int parent_process(int semid, int cycles);

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
        fprintf(stderr, "Negative cycles number: %d\n", cycles);
        exit(EXIT_FAILURE);
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    /* Create shared memory */  
    if (create_shared_memory()) {
        cleanup();
        exit(EXIT_FAILURE);
    }

    /* Create semaphore */
    if (create_semaphore()) {
        cleanup();
        exit(EXIT_FAILURE);
    }
    
    /* Create child process*/
    for (int i = 0; i < CHLD_COUNT; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            cleanup();
            exit(EXIT_FAILURE);
        }
        /* Child process */
        if (pid == 0) {
            close(pipefd[0]);
            int result = child_process(pipefd[1], cycles);
            close(pipefd[1]);

            exit(result);
        }
    }
    
    /* Parent process */
    close(pipefd[1]);
    int result = parent_process(pipefd[0], cycles * CHLD_COUNT);
    close(pipefd[0]);

    for (int i = 0; i < CHLD_COUNT; i++) {
        wait(NULL); 
    }

    cleanup();
    exit(result);      
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

void cleanup() {
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }
    if (shmid != -1) {
        shmdt(read_count);
        shmctl(shmid, IPC_RMID, NULL);
    }
}

int sem_op(int semnum, int op) {
    struct sembuf sb;
    sb.sem_num = semnum;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int create_shared_memory() {
    shmid = shmget(IPC_PRIVATE, sizeof(unsigned int), IPC_CREAT | PERMISSIONS);
    if (shmid == -1) {
        perror("shmget failed");
        return EXIT_FAILURE;
    }

    read_count = shmat(shmid, NULL, 0);
    if (read_count == (void *) -1) {
        perror("shmat failed");
        return EXIT_FAILURE;
    }
    *read_count = 0;

    return EXIT_SUCCESS;
}

int create_semaphore() {
    semid = semget(IPC_PRIVATE, SEM_COUNT, IPC_CREAT | PERMISSIONS);
    if (semid == -1) {
        perror("semget failed");
        return EXIT_FAILURE;
    }

    semun arg;
    arg.val = SIGNAL; 
    if (semctl(semid, READER, SETVAL, arg) == -1 || semctl(semid, WRITER, SETVAL, arg) == -1) {
        perror("semctl failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int lock_semaphore() {
    /* LOCK READERS */
    if (sem_op(READER, WAIT)){
        return EXIT_FAILURE;
    }   
    (*read_count)++;

    /* LOCK WRITER */
    if (*read_count == 1) {
        if (sem_op(WRITER, WAIT)) {
            return EXIT_FAILURE;
        }
    }

    /* UNLOCK READERS */
    if (sem_op(READER, SIGNAL)) {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int unlock_semaphore() {
    /* LOCK READERS */
    if (sem_op(READER, WAIT) != EXIT_SUCCESS){
        return EXIT_FAILURE;
    }
    (*read_count)--;

    /* UNLOCK WRITER */
    if (*read_count == 0) {
        if (sem_op(WRITER, SIGNAL) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
                
    /* UNLOCK READERS */
    if (sem_op(READER, SIGNAL)!= EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int child_process(int pipefd, int cycles) {  
     
    srand(time(NULL) ^ getpid());
        
    for (int i = 0; i < cycles; i++) {

        /* LOCK SEMAPHORE */
        if (lock_semaphore(semid)){
            return EXIT_FAILURE;
        }
                 
        /* Read file */
        char msg[128];
        int fd = open(OUTPUT_FILE, O_RDONLY);
        if (fd < 0) {
            snprintf(msg, sizeof(msg), "\nChild process %d: File %s doesn't exists or empty.\n", getpid(), OUTPUT_FILE);
            write_stdout(msg);
        } 
        else {                             
            char buffer[256];
            ssize_t bytesRead;         
            snprintf(msg, sizeof(msg), "\nChild process %d: Read file %s:\n", getpid(), OUTPUT_FILE);
            write_stdout(msg);
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
        if (unlock_semaphore(semid)) {
            return EXIT_FAILURE;
        }
                 
        /* Generate random number*/
        int new_num = rand();
        snprintf(msg, sizeof(msg), "Child process %d: Send new number: %d\n", getpid(), new_num);
        write_stdout(msg);

        /* Send random number to parent process */
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
        
        /* Received random number from child process */
        int received;
        if (read(pipefd, &received, sizeof(received)) != sizeof(received)) {
            perror("pipe failed");
            return EXIT_FAILURE;
        }
        
        /* LOCK SEMAPHORE */
        if (sem_op(WRITER, WAIT)) {
            return EXIT_FAILURE;
        };
  
        char msg[128];
        snprintf(msg, sizeof(msg), "\nParent process: Received number: %d\n", received);
        write_stdout(msg);
        
        /* Write in file */
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
        if (sem_op(WRITER, SIGNAL)) {
            return EXIT_FAILURE;
        };
        
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}
