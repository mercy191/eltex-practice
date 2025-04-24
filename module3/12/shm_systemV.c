#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define SHM_KEY_PATH    "/"    
#define SEM_KEY_PATH    "/" 
#define SHM_KEY_ID      31 
#define SEM_KEY_ID      32      
#define PERMISSIONS     (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define MAX_BUF         128
#define WAIT            -1
#define SIGNAL          1
#define WAIT_TIME       5

typedef struct shm_data {
    int numbers[MAX_BUF];
    int count;
    int min;
    int max;
} shm_data ;

int         shmid           = -1;
int         semid           = -1;
int         sets_processed  = 0;
int         running         = 1;
shm_data*   data            = NULL;
key_t       shm_key         = -1;
key_t       sem_key         = -1;


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Create process key */
int create_shared_key();

/* Create process key */
int create_sem_key();

/* Create shared memory */
int create_shared_memory(key_t key);

/* Create semaphore */
int create_semaphore(key_t key);

/* Switch semaphore*/
int sem_op(int op);

/* Clean resources */
void cleanup();

/* Child process */
int child_process();

/* Parent process */
int parent_process(int child_pid);


int main()
{
    signal(SIGINT, handle_sigint);

    if (create_shared_key() == -1) {
        exit(EXIT_FAILURE);
    }

    if (create_sem_key() == -1) {
        exit(EXIT_FAILURE);
    }

    if (create_shared_memory(shm_key) == -1) {
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (create_semaphore(sem_key) == -1) {
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

int create_shared_key() {
    if ((shm_key = ftok(SHM_KEY_PATH, SHM_KEY_ID)) == -1) {
        perror("ftok");
        return -1;
    }

    return 0;
}

int create_sem_key() {
    if ((shm_key = ftok(SEM_KEY_PATH, SEM_KEY_ID)) == -1) {
        perror("ftok");
        return -1;
    }

    return 0;
}

int create_shared_memory(key_t key) {
    shmid = shmget(key, sizeof(shm_data), IPC_CREAT | PERMISSIONS);
    if (shmid == -1) {
        perror("shmget failed");
        return -1;
    }

    data = (shm_data*)shmat(shmid, NULL, 0);
    if (data == (shm_data*)(-1)) {
        perror("shmat failed");
        return -1;
    }

    return 0;
}

int create_semaphore(key_t key) {
    semid = semget(key, 1, IPC_CREAT | PERMISSIONS);
    if (semid == -1) {
        perror("semget failed");
        return -1;
    }

    if (semctl(semid, 0, SETVAL, SIGNAL) == -1) {
        perror("semctl failed");
        return -1;
    }

    return 0;
}

int sem_op(int op) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        return -1;
    }
    return 0;
}

void cleanup() {
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }

    if (shmid != -1) {
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
    }
}

int child_process() {
    while (running) {
        /* Lock semaphore */
        if (sem_op(WAIT) == -1) {
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
        if (sem_op(SIGNAL) == -1) {
            return EXIT_FAILURE;
        };

        sleep(1);
    }

    return EXIT_SUCCESS;  
}

int parent_process(int child_pid) {
    while (running) {
        /* Lock semaphore */
        if (sem_op(WAIT) == -1) {
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
        if (sem_op(SIGNAL) == -1) {
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

