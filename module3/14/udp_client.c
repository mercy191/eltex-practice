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
int                 client_sockfd = -1;
uint16_t            server_port = 0;
struct sockaddr_in  server_addr;
struct sockaddr_in  client_addr;
pthread_t           recv_thread;
pthread_t           send_thread;


/* SIGINT handle, send stop message */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Listen socket */
void* listen_sock(void *arg);

/* Send message in socket */
void* speak_sock(void *arg);

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_sigint);

    if (argc != 3) {
        fprintf(stderr, "Usage: udp_client <server IP address> <server port>\n");
        exit(EXIT_FAILURE);
    }

    server_port = (uint16_t)strtol(argv[2], NULL, 10);

    if ((client_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(0);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(client_sockfd, (struct sockaddr*) &client_addr, sizeof(client_addr)) == -1) {
        perror("bind failed");
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_aton(argv[1], &server_addr.sin_addr) == 0) {
        perror("Invalid IP adress");
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&recv_thread, NULL, listen_sock, NULL) != 0) {
        perror("pthread_create failed");
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, speak_sock, NULL) != 0) {
        perror("pthread_create failed");
        running = 0;
        pthread_cancel(recv_thread); 
        pthread_join(recv_thread, NULL);
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(client_sockfd);
    exit(EXIT_SUCCESS);
}

void handle_sigint() {
    running = 0; 
    pthread_cancel(recv_thread);
    pthread_cancel(send_thread);  
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

void* listen_sock(void *arg) {
    struct sockaddr_in recvaddr;
    socklen_t recvaddrlen = sizeof(recvaddr);
    char bufline[MAX_BUF];  

    while (running) {  
        pthread_testcancel();

        memset(bufline, 0, sizeof(bufline));
        if (recvfrom(client_sockfd, bufline, MAX_BUF, 0, (struct sockaddr*) &recvaddr, &recvaddrlen) == -1) {
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

void* speak_sock(void *arg) {
    char bufline[MAX_BUF];  

    while (running) {      
        pthread_testcancel();

        memset(bufline, 0, sizeof(bufline));
        fgets(bufline, MAX_BUF, stdin);
        if (sendto(client_sockfd, bufline, strlen(bufline) + 1, 0, (struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr)) == -1) {
            perror("sendto failed");
            break;
        }
    } 
    return NULL;  
}

