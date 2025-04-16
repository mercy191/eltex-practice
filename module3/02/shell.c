#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_BUF 512 
#define MAX_ARGS 32

int main() 
{
    char buf[MAX_BUF];
    char *args[MAX_ARGS];

    while(1) {
        printf("shell> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            break;
        }

        buf[strcspn(buf, "\n")] = '\0';
        if (strcmp(buf, "exit") == 0) {
            break;
        }

        int count = 0;
        char* token = strtok(buf, " ");
        while (token != NULL && count < MAX_ARGS - 1) {
            args[count++] = token;
            token = strtok(NULL, " ");
        }
        args[count] = NULL;

        if (count == 0) {
            continue;
        }

        /* Create child process*/
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork() error");
            exit(EXIT_FAILURE);
        }
        /* Child process */
        else if (pid == 0) {
            if (strchr(args[0], '/') == NULL) {
                char commandPath[MAX_BUF];
                snprintf(commandPath, sizeof(commandPath), "./%s", args[0]);
                execvp(commandPath, args);
            } else {
                execvp(args[0], args);
            }
            perror("exec");
            exit(EXIT_FAILURE);
        } 
        /* Parent process */
        else {
            wait(NULL);
        }
    }

    exit(EXIT_SUCCESS);
}