#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define MAX_BUF 1000

volatile int        running = 1;
int                 sockfd = -1;
uint16_t            servport = 0;
struct sockaddr_in  serveraddr;
struct sockaddr_in  clientaddr;
pthread_t           recvthread;
pthread_t           sendthread;


/* SIGINT handle, send stop message */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Listen socket */
void*  listen_thread(void *arg);

/* Send message in socket */
void*  send_thread(void *arg);

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_sigint);

    if (argc != 3) {
        fprintf(stderr, "Usage: udp_client <server IP address> <server port>\n");
        exit(EXIT_FAILURE);
    }

    servport = (uint16_t)strtol(argv[2], NULL, 10);

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(0);
    clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*) &clientaddr, sizeof(clientaddr)) == -1) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(servport);
    if (inet_aton(argv[1], &serveraddr.sin_addr) == 0) {
        perror("Invalid IP adress");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&recvthread, NULL, listen_thread, NULL) != 0) {
        perror("pthread_create failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&sendthread, NULL, send_thread, NULL) != 0) {
        perror("pthread_create failed");
        running = 0;
        pthread_cancel(recvthread); 
        pthread_join(recvthread, NULL);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_join(recvthread, NULL);
    pthread_join(sendthread, NULL);

    close(sockfd);
    exit(EXIT_SUCCESS);
}

void handle_sigint() {
    running = 0; 
    pthread_cancel(recvthread);
    pthread_cancel(sendthread);  
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

void* listen_thread(void *arg) {
    struct sockaddr_in recvaddr;
    socklen_t recvaddrlen = sizeof(recvaddr);
    char bufline[MAX_BUF];  

    while (running) {  
        pthread_testcancel();

        memset(bufline, 0, sizeof(bufline));
        if (recvfrom(sockfd, bufline, MAX_BUF, 0, (struct sockaddr*) &recvaddr, &recvaddrlen) == -1) {
            perror("recvfrom failed");
            break;
        }
    
        char msg[2*MAX_BUF];
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &recvaddr.sin_addr, ip_str, INET_ADDRSTRLEN);
        snprintf(msg, sizeof(msg), "< (%s) %s", ip_str, bufline);
        write_stdout(msg);
    }

    return NULL;
}

void* send_thread(void *arg) {
    char bufline[MAX_BUF];  

    while (running) {      
        pthread_testcancel();

        memset(bufline, 0, sizeof(bufline));
        fgets(bufline, MAX_BUF, stdin);
        if (sendto(sockfd, bufline, strlen(bufline) + 1, 0, (struct sockaddr*) &serveraddr, (socklen_t) sizeof(serveraddr)) == -1) {
            perror("sendto failed");
            break;
        }
    } 
    return NULL;  
}

