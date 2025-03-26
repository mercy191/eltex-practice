#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define ERROR_IP 0
 

__uint32_t matchIP(char* ip) {

    char buffer[36];
    strncpy(buffer, ip, sizeof(buffer));
    __uint32_t result = 0;

    char* token = strtok(buffer, ".");
    for (int i = 0; i < 4; i++) {
        if (token == NULL)
            return ERROR_IP;

        __uint8_t octet = atoi(token);
        if (octet > 255)    
            return ERROR_IP;

        result = (result << 8) | (octet & 0377);
        token = strtok(NULL, ".");
    }

    if (strtok(NULL, ".") != NULL)
        return ERROR_IP;

    return result;
}

__uint32_t generateIP() {
    __uint32_t random_ip = 0;
    for (int i = 0; i < 2; i++){
        random_ip = (random_ip << 8) | (rand() % 256);
    }

    return random_ip | 0xC0A80000;
}   


int main(int argc, char* argv[]) 
{   
    if (argc != 4) {
        return -1;
    }
   
    __uint32_t gw_ip_addr = matchIP(argv[1]);
    __uint32_t mask_ip_addr = matchIP(argv[2]);
    __uint32_t gw_newtwork = gw_ip_addr & mask_ip_addr;

    __uint64_t N = (__uint64_t)atoi(argv[3]);
    __uint64_t local_count   = 0; 
    __uint64_t foreign_count = 0; 

    srand((__uint32_t)time(NULL));

    for (__uint64_t i = 0; i < N; i++) {

        __uint32_t random_ip_addr = generateIP();
        __uint32_t randon_network = random_ip_addr & mask_ip_addr;

        if (randon_network == gw_newtwork) {
            local_count++;
        }
    }
    foreign_count = N - local_count;

    double local_percent   = 100.0 * local_count   / N;
    double foreign_percent = 100.0 * foreign_count / N;

    printf("Обработано пакетов: %ld\n", N);
    printf("В своей подсети: %ld (%.2f%%)\n", local_count, local_percent);
    printf("В чужих сетях:   %ld (%.2f%%)\n", foreign_count, foreign_percent);

    return 0;
}