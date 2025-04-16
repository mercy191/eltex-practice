#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char* argv[]) 
{
    if (argc < 2) {
        fprintf(stderr, "Error input arguments, usage: %s side_length1, side_length2, ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int total_sides = argc - 1;
    int half_sides  = total_sides / 2;

    /* Create child process */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }
    /* Child process */
    else if (pid == 0) {
        for (int i = 1; i <= half_sides; i++) {
            double side_length = atof(argv[i]);
            double area = side_length * side_length;
            printf("The square with side %.3f has an area of %.3f cm, child process\n", side_length, area);
        }
        exit(EXIT_SUCCESS);
    }
    /* Parent process */
    else {
        //wait(NULL);
        for (int i = half_sides + 1; i <= total_sides; i++) {
            double side_length = atof(argv[i]);
            double area = side_length * side_length;
            printf("The square with side %.3f has an area of %.3f cm, parent process\n", side_length, area);
        }
    }

    exit(EXIT_SUCCESS);
}