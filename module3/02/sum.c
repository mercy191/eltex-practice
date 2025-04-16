#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
    if (argc < 2) {
        fprintf(stderr, "Error input arguments, usage: %s num1 num2 ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int sum = 0;
    for (int i = 1; i < argc; i++) {
        sum += atoi(argv[i]);
    }
    printf("Sum: %d\n", sum);    

    exit(EXIT_SUCCESS);
}