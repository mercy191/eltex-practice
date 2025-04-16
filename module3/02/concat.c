#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
    if (argc < 2) {
        fprintf(stderr, "Error input arguments, usage: %s str1 str2 ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
    }
    printf("\n");
    
    exit(EXIT_SUCCESS);
}