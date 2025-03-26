#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>


#define ADD         0x001
#define SET         0x002
#define DELETE      0x003

#define USER        0x1C0
#define GROUP       0x038
#define OTHER       0x007
#define ALL         0x1FF

#define READ        0x124
#define WRITE       0x092
#define EXEC        0x049
#define NO          0x000

#define ERROR       0xFFF


void letterToBit(char* c, unsigned int* permission);
void numericToBit(char* c, unsigned int* permission);
void letterPermissions(unsigned int mode, char *permission);
void bitPermissions(unsigned int mode, unsigned int *permissions);
void numericPermissions(unsigned int mode, unsigned int *permission);

unsigned int badchmod(char *c, const char *filename);
unsigned int letterchmod(char *c, unsigned int mode);
unsigned int numericchmod(char* c, unsigned int mode);
unsigned int matchPattern(char *input, const char *pattern);

#pragma region /* Пункт 1 */

void letterToBit(char* c, unsigned int* permission) {
    for (int i = 0; i < 9; i++) {
        if (c[i] == '-'){
            permission[i] = 0;
        }
        else {
            permission[i] = 1;
        }
    }
}

void numericToBit(char* c, unsigned int* permission) {
    unsigned int mask = 0x1C0;
    unsigned int mode = 0x000;

    for (int i = 0; i < 3; i++) {
            switch(c[i]) 
            {
                case '0':
                    mode |= mask & NO;
                    break;
                
                case '1':
                    mode |= mask & WRITE;
                    break;

                case '2':
                    mode |= mask & WRITE;
                    break;

                case '4':
                    mode |= mask & READ;
                    break;

                case '5':
                    mode |= mask & (READ | EXEC);
                    break;

                case '6':
                    mode |= mask & (READ | WRITE);
                    break;

                case '7':
                    mode |= mask & (READ | WRITE | EXEC);
                    break;

                default:
                    break;
            }
            mask = mask >> 3;
    }

    for (int i = 0; i < 9; i++) {
        permission[9 - 1 - i] = (mode >> i) & 1;
    }

}

#pragma endregion


#pragma region /* Пункт 2*/

void letterPermissions(unsigned int mode, char *permission) {

    if (mode & S_IRUSR) permission[0] = 'r';
    if (mode & S_IWUSR) permission[1] = 'w';
    if (mode & S_IXUSR) permission[2] = 'x';

    if (mode & S_IRGRP) permission[3] = 'r';
    if (mode & S_IWGRP) permission[4] = 'w';
    if (mode & S_IXGRP) permission[5] = 'x';

    if (mode & S_IROTH) permission[6] = 'r';
    if (mode & S_IWOTH) permission[7] = 'w';
    if (mode & S_IXOTH) permission[8] = 'x';
}

void numericPermissions(unsigned int mode, unsigned int *permission) {
    unsigned int mask = 0x007;

    for (int i = 2; i >= 0; i--) {
        permission[i] = mode & mask;
        mode = mode >> 3;
    }
}

void bitPermissions(unsigned int mode, unsigned int *permission) {
    
    unsigned int mask = 0x100;

    for (int i = 0; i < 9; i++) {
        if (mode & mask) permission[i] = 1;
        else permission[i] = 0;
        mask = mask >> 1;
    }
}

#pragma endregion


#pragma region /* Пункт 3 */

unsigned int badchmod(char *c, const char *filename) {

    struct stat file_stat;
    unsigned int mode = 0x000;

    if (stat(filename, &file_stat) == 0)
        mode = file_stat.st_mode & 0x1FF;   
    else
        return (unsigned int)ERROR;
    

    if (matchPattern(c, "^[ugoar]+[+=-](r|w|x){1,3}$"))
        return (unsigned int)letterchmod(c, mode);
    else if (matchPattern(c, "^[0-7]{3}$"))
        return (unsigned int)numericchmod(c, mode);
    else
        return (unsigned int)ERROR;  

}

unsigned int letterchmod(char *c, unsigned int mode) {

    unsigned int access_bit = 0x000;
    unsigned int action_bit = 0x000;
    unsigned int permission = 0;

    for (int i = 0; i < strlen(c); i++) 
    {
        switch (c[i])
        {
            case '+':
                permission = ADD;
                break;
            case '-':
                permission = DELETE;
                break;
            case '=':
                permission = SET;
                break;
    
            case 'u':
                access_bit |= USER;
                break;
            case 'g':
                access_bit |= GROUP;
                break;
            case 'o': 
                access_bit |= OTHER;
                break;
            case 'a':
                access_bit |= ALL;
                break;
    
            case 'r':
                action_bit |= READ;
                break;
            case 'w':
                action_bit |= WRITE;
                break;
            case 'x':
                action_bit |= EXEC;
                break;
                    
            default:
                return (unsigned int)ERROR;
                break;
        }   
    }
    switch (permission)
    {
    case ADD:
        mode |= access_bit & action_bit;
        break;
    case SET:
        mode = access_bit & action_bit;
        break;    
    case DELETE:
        mode ^= access_bit & action_bit;
        break;

    default:
        return (unsigned int)ERROR;
        break;
    }

    return mode;
}

unsigned int numericchmod(char* c, unsigned int mode) {

    unsigned int mask = 0x1C0;

    for (int i = 0; i < strlen(c); i++) 
    {
        switch(c[i]) 
        {
            case '0':
                mode |= mask & NO;
                break;

            case '1':
                mode |= mask & WRITE;
                break;

            case '2':
                mode |= mask & WRITE;
                break;
                
            case '4':
                mode |= mask & READ;
                break;

            case '5':
                mode |= mask & (READ | EXEC);
                break;

            case '6':
                mode |= mask & (READ | WRITE);
                break;

            case '7':
                mode |= mask & (READ | WRITE | EXEC);
                break;

            default:
                return (unsigned int)ERROR;
                break;

        }
        mask = mask >> 3;     
    }

    return mode;
}

#pragma endregion

unsigned int matchPattern(char *input, const char *pattern) {
    regex_t regex;
    unsigned int reti;

    reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti) return 0;

    reti = regexec(&regex, input, 0, NULL, 0);
    regfree(&regex);

    if (!reti) return 1; 
    else if (reti == REG_NOMATCH) return 0;
    else return 0; 
}


int main() 
{
    char mode1[] = "r-xr-xr-x";
    unsigned int permission1[9];
    printf("Letter to bit r-xr-xr-x: \t"); 
    letterToBit(mode1, permission1);
    for (int i = 0; i < 9; i++) {
        printf("%d ", permission1[i]);
    }
    printf("\n");

    char mode2[] = "555";
    unsigned int permission2[9];
    printf("Numeric to bit 555: \t\t");  
    numericToBit(mode2, permission2);
    for (int i = 0; i < 9; i++) {
        printf("%d ", permission2[i]);
    }
    printf("\n\n");


    const char* filename = "temp_file.txt";
    struct stat file_stat;
    stat(filename, &file_stat);
    unsigned int mode = file_stat.st_mode & 0x1FF;
    printf("File %s mode: \t%x", filename, mode);
    printf("\n\n"); 


    char permission3[9] = "---------";
    printf("Letter permission %x: \t\t", mode); 
    letterPermissions(mode, permission3);
    for (int i = 0; i < 9; i++) {
        printf("%c ", permission3[i]);
    }
    printf("\n");

    unsigned int permission4[3];
    printf("Numeric permission %x: \t", mode);    
    numericPermissions(mode, permission4);
    for (int i = 0; i < 3; i++) {
        printf("%d ", permission4[i]);
    }
    printf("\n");

    unsigned int permission5[9];
    printf("Bit permission %x: \t\t", mode);
    bitPermissions(mode, permission5);
    for (int i = 0; i < 9; i++) {
        printf("%d ", permission5[i]);
    }
    printf("\n\n");

    unsigned int new_mode = badchmod("u+x", filename);
    printf("chmod u+x %s: \t%x\n", filename, badchmod("u+x", filename));
    printf("chmod 764 %s: \t%x\n", filename, badchmod("764", filename));
    printf("\n");

    char permission6[9] = "---------";
    printf("Letter permission %x: \t\t", new_mode); 
    letterPermissions(new_mode, permission6);
    for (int i = 0; i < 9; i++) {
        printf("%c ", permission6[i]);
    }
    printf("\n");

    unsigned int permission7[3];
    printf("Numeric permission %x: \t", new_mode);    
    numericPermissions(new_mode, permission7);
    for (int i = 0; i < 3; i++) {
        printf("%d ", permission7[i]);
    }
    printf("\n");

    unsigned int permission8[9];
    printf("Bit permission %x: \t\t", new_mode);
    bitPermissions(new_mode, permission8);
    for (int i = 0; i < 9; i++) {
        printf("%d ", permission8[i]);
    }
    printf("\n\n");
}