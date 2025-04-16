#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

#define OUTPUT_FILE     "output.txt"
#define WRITER_SEM_NAME "/writer_sem"
#define READER_SEM_NAME "/reader_sem"
#define READ_COUNT_SHM  "/read_count_shm"
#define PERMISSIONS     (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define CHLD_COUNT      2

unsigned int*   read_count = NULL;
int             shm_fd = -1;
sem_t*          writer_sem;
sem_t*          reader_sem;

/* Write in STDOUT */
void write_stdout(const char *str);

/* Clean resources */
void cleanup();

/* Create shared memory */
int create_shared_memory();

/* Create semaphores */
int create_semaphores();

/* Child process */
int child_process(int pipefd, int cycles);

/* Parent process */
int parent_process(int pipefd, int cycles);

/* Lock semaphore */
int lock_semaphore();

/* Unlock semaphore */
int unlock_semaphore();


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

    /* Create semaphores */
    if (create_semaphores()) {
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

            sem_close(writer_sem);
            sem_close(reader_sem);
            close(shm_fd);
            munmap(read_count, sizeof(unsigned int));          
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
    if (read_count != NULL && read_count != MAP_FAILED) {
        if (munmap(read_count, sizeof(unsigned int)) == -1) {
            perror("munmap failed");
        }
        read_count = NULL;
    }

    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(READ_COUNT_SHM);
        shm_fd = -1;
    }

    if (writer_sem != NULL && sem_close(writer_sem) == -1) {
        perror("sem_close failed");
    }
    if (reader_sem != NULL && sem_close(reader_sem) == -1) {
        perror("sem_close failed");
    }

    if (getpid() == 1) {  
        if (sem_unlink(WRITER_SEM_NAME) == -1) {
            perror("sem_unlink failed");
        }
        if (sem_unlink(READER_SEM_NAME) == -1) {
            perror("sem_unlink failed");
        }
    }
}

int create_shared_memory() {
    shm_fd = shm_open(READ_COUNT_SHM, O_CREAT | O_RDWR, PERMISSIONS);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return EXIT_FAILURE;
    }
 
    if (ftruncate(shm_fd, sizeof(unsigned int)) == -1) {
        perror("ftruncate failed");
        close(shm_fd);
        return EXIT_FAILURE;
    }
 
    read_count = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (read_count == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        return EXIT_FAILURE;
    }
    *read_count = 0;
 
    return EXIT_SUCCESS;
}

int create_semaphores() {
    writer_sem = sem_open(WRITER_SEM_NAME, O_CREAT, PERMISSIONS, 1);
    reader_sem = sem_open(READER_SEM_NAME, O_CREAT, PERMISSIONS, 1);
    if (writer_sem == SEM_FAILED || reader_sem == SEM_FAILED) {
        perror("sem_open failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int lock_semaphore() {
    /* LOCK READERS */   
    if (sem_wait(reader_sem) == -1){
        return EXIT_FAILURE;
    }   
    (*read_count)++;

    /* LOCK WRITER */
    if (*read_count == 1) {
        if (sem_wait(writer_sem) == -1) {
            return EXIT_FAILURE;
        }
    }

    /* UNLOCK READERS */
    if (sem_post(reader_sem) == -1) {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int unlock_semaphore() {
    /* LOCK READERS */
    if (sem_wait(reader_sem) == -1){
        return EXIT_FAILURE;
    }
    (*read_count)--;

    /* UNLOCK WRITER */
    if (*read_count == 0) {
        if (sem_post(writer_sem) == -1) {
            return EXIT_FAILURE;
        }
    }
                
    /* UNLOCK READERS */
    if (sem_post(reader_sem) == -1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int child_process(int pipefd, int cycles) {  
     
    srand(time(NULL) ^ getpid());
        
    for (int i = 0; i < cycles; i++) {

        /* LOCK SEMAPHORE */
        if (lock_semaphore()) {
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
        if (unlock_semaphore()) {
            return EXIT_FAILURE;
        }
                 
        /* Generate random number*/
        int new_num = rand();
        snprintf(msg, sizeof(msg), "Child process %d: Send new number: %d\n", getpid(), new_num);
        write_stdout(msg);

        /* Send random number to parent process */
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
        
        /* Received random number from child process */
        int received;
        if (read(pipefd, &received, sizeof(received)) != sizeof(received)) {
            perror("pipe");
            return EXIT_FAILURE;
        }
        
        /* LOCK SEMAPHORE */
        if (lock_semaphore()) {
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
        if (unlock_semaphore()) {
            return EXIT_FAILURE;
        };
        
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}
