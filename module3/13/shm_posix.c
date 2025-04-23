#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define SHM_NAME        "/posix_shm"
#define SEM_NAME        "/posix_sem"   
#define PERMISSIONS     (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define MAX_BUF         128
#define WAIT_TIME       5

typedef struct shm_data {
    int numbers[MAX_BUF];
    int count;
    int min;
    int max;
} shm_data ;

int         shm_fd          = -1;
int         sets_processed  = 0;
int         running         = 1;
shm_data*   data            = NULL;
sem_t*      sem             = NULL;


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Create shared memory */
int create_shared_memory();

/* Create semaphore */
int create_semaphore();

/* Clean resources */
void cleanup();

/* Child process */
int child_process();

/* Parent process */
int parent_process(int child_pid);


int main()
{
    signal(SIGINT, handle_sigint);

    if (create_shared_memory() == -1) {
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (create_semaphore() == 1) {
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
        int result = child_process();

        sem_close(sem);
        close(shm_fd);
        exit(result);
    }
    /* Parent process */
    else {
        int result = parent_process(pid);

        cleanup();
        exit(result);
    }
}

void handle_sigint() {
    running = 0;
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int create_shared_memory() {
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, PERMISSIONS);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return -1;
    }
  
    if (ftruncate(shm_fd, sizeof(shm_data)) == -1) {
        perror("ftruncate failed");
        return -1;
    }
  
    data = mmap(NULL, sizeof(shm_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }
  
    return 0;
}

int create_semaphore() {    
    sem = sem_open(SEM_NAME, O_CREAT, PERMISSIONS, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return -1;
    }

    return 0;
}

void cleanup() {
    if (data != NULL && data != MAP_FAILED) {
        if (munmap(data, sizeof(unsigned int)) == -1) {
            perror("munmap failed");
        }
        data = NULL;
    }

    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHM_NAME);
        shm_fd = -1;
    }

    if (sem != NULL && sem_close(sem) == -1) {
        perror("sem_close failed");
    }

    if (getpid() == 1) {  
        if (sem_unlink(SEM_NAME) == -1) {
            perror("sem_unlink failed");
        }
    }
}

int child_process() {
    while (running) {
        /* Lock semaphore */
        if (sem_wait(sem) == -1) {
            return EXIT_FAILURE;
        };

        int min = data->numbers[0];
        int max = data->numbers[0];
        for (int i = 1; i < data->count; i++) {
            if (data->numbers[i] < min) min = data->numbers[i];
            if (data->numbers[i] > max) max = data->numbers[i];
        }
        data->min = min;
        data->max = max;

        /* Unlock semaphore */
        if (sem_post(sem) == -1) {
            return EXIT_FAILURE;
        };

        sleep(1);
    }

    return EXIT_SUCCESS;  
}

int parent_process(int child_pid) {
    while (running) {
        /* Lock semaphore */
        if (sem_wait(sem) == -1) {
            return EXIT_FAILURE;
        };

        data->count = rand() % 10 + 1;
        for (int i = 0; i < data->count; i++) {
            data->numbers[i] = rand() % 100;
        }

        char msg[MAX_BUF];
        snprintf(msg, sizeof(msg), "Set %d:\n", sets_processed + 1);
        write_stdout(msg);

        write_stdout("Numbers: ");
        for (int i = 0; i < data->count; i++) {
            snprintf(msg, sizeof(msg), "%d ", data->numbers[i]);
            write_stdout(msg);
        }
        snprintf(msg, sizeof(msg), "\nMin: %d, Max: %d\n\n", data->min, data->max);
        write_stdout(msg);

        sets_processed++;

        /* Unlock semaphore */
        if (sem_post(sem) == -1) {
            return EXIT_FAILURE;
        };

        sleep(1);
    }

    char msg[MAX_BUF];
    snprintf(msg, sizeof(msg), "\nTotal sets processed: %d\n", sets_processed);
    write_stdout(msg);  

    /* Send SIGINT */
    if (kill(child_pid, SIGINT) == -1) {
        perror("kill failed");
        return EXIT_FAILURE;
    }

    int waited = 0;
    int status = 0;
    while (waited < WAIT_TIME) {
        pid_t pid = waitpid(pid, &status, WNOHANG);
        if (pid == 0) {
            sleep(1);
            waited++;
        } 
        else if (pid == child_pid) {
            return EXIT_SUCCESS;
        } 
        else {
            perror("waitpid failed");
            return EXIT_FAILURE;
        }
    }
    
    /* Send SIGKILL */
    if (kill(child_pid, SIGKILL) == -1) {
        perror("kill failed");
        return EXIT_FAILURE;
    }
    waitpid(child_pid, &status, 0);

    return EXIT_SUCCESS;  
}

