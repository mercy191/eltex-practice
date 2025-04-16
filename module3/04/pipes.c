#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        fprintf(stderr, "Error input arguments, usage: %s <number_of_numbers>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int count = atoi(argv[1]);
    if (count <= 0) {
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

        srand(time(NULL));
        for (int i = 0; i < count; i++) {
            int num = rand();
            if (write(pipefd[1], &num, sizeof(num)) != sizeof(num)) {
                perror("write");
                close(pipefd[1]);
                exit(EXIT_FAILURE);
            }
        }

        close(pipefd[1]);
        exit(EXIT_SUCCESS);
    }
    /* Рarent process */
    else {
        close(pipefd[1]);
        
        for (int i = 0; i < count; i++) {
            int num;
            ssize_t bytesRead = read(pipefd[0], &num, sizeof(num));
            if (bytesRead == -1) {
                perror("read");
                break;
            } 
            else if (bytesRead == 0) {
                break;
            }
            printf("Number %d: %d\n", i, num);
        }

        close(pipefd[0]);
        wait(NULL);
    }

    exit(EXIT_SUCCESS);
}
