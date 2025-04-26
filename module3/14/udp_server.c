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

#define MAX_BUF     1000
#define CLI_COUNT   3

struct socksaddr_in {
    struct sockaddr_in clientaddr;
    int active;
};

volatile int            running = 1;
int                     client_count = 0;
int                     server_sockfd = -1;
uint16_t                server_port = 0;
struct sockaddr_in      server_addr;
struct socksaddr_in     clientsaddr[CLI_COUNT] = {0};
pthread_t               proc_thread;

/* SIGINT handle, send stop message */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Comprassion of clients IP adresses and ports */
int clients_equal(struct sockaddr_in *a, struct sockaddr_in *b);

/* Checks for a client. Returns 1 if it exists, 0 otherwise. */
int is_exists(struct socksaddr_in *clientsaddr, struct sockaddr_in *recvaddr);

/* Adds client. If there is space, returns the index in the array, otherwise -1. */
int add_clientaddr(struct socksaddr_in *clientsaddr, struct sockaddr_in *addr);

/* Listen socket */
int listen_sock(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen, char *bufline);

/* Send message in socket */
int send_sock(int sockfd, struct sockaddr_in *addr, socklen_t addrlen, char *bufline);

/* Deals with the processing and sending of messages */
void* processing_thread(void *arg); 

int main(int argc, char* argv[]) 
{
    signal(SIGINT, handle_sigint);  

    if (argc != 2) {
        fprintf(stderr, "Usage: udp_server <server port>\n");
        exit(EXIT_FAILURE);
    }

    server_port = (uint16_t)strtol(argv[1], NULL, 10);

    if ((server_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket failed"); 
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&proc_thread, NULL, processing_thread, NULL) != 0) {
        perror("pthread_create failed");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_join(proc_thread, NULL);

    close(server_sockfd);
    exit(EXIT_SUCCESS);
}

void handle_sigint() {
    running = 0; 
    pthread_cancel(proc_thread);
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int clients_equal(struct sockaddr_in *a, struct sockaddr_in *b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

int is_exists(struct socksaddr_in *clientsaddr, struct sockaddr_in *recvaddr) {
    for (int i = 0; i < CLI_COUNT; i++) {
        if (clientsaddr[i].active && clients_equal(recvaddr, &clientsaddr[i].clientaddr)) {
            return 1;
        }
    }
    return 0;
}

int add_clientaddr(struct socksaddr_in *clientsaddr, struct sockaddr_in *addr) {
    for (int i = 0; i < CLI_COUNT; i++) {
        if (!clientsaddr[i].active) {
            memcpy(&clientsaddr[i].clientaddr, addr, sizeof(addr));
            clientsaddr[i].active = 1;                    
            return i;
        }
    } 
     
    return -1;
}

int listen_sock(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen, char *bufline) {   
    memset(bufline, 0, MAX_BUF);      
    if (recvfrom(sockfd, bufline, MAX_BUF, 0, (struct sockaddr *) addr, addrlen) == -1) {
        perror("recvfrom failed");
        return -1;
    }

    return 0;
}

int send_sock(int sockfd, struct sockaddr_in *addr, socklen_t addrlen, char *bufline) {
    if (sendto(sockfd, bufline, strlen(bufline), 0, (struct sockaddr*) addr, addrlen) == -1) {
        perror("sendto failed");
        return -1;
    }

    return 0;
}

void* processing_thread(void *arg) {
    char                bufline[MAX_BUF];
    struct sockaddr_in  recvaddr;
    socklen_t           recvaddrlen = sizeof(recvaddr);

    /* Process one message at a time */
    while (running) {
        pthread_testcancel();

        if (listen_sock(server_sockfd, &recvaddr, &recvaddrlen, bufline) == -1) {
            break;
        }

        if (!is_exists(clientsaddr, &recvaddr) && client_count < CLI_COUNT) {             
            int index = -1;
            char msg[MAX_BUF];
            if ((index = add_clientaddr(clientsaddr, &recvaddr)) != -1) {
                client_count++;
                snprintf(msg, sizeof(msg), "New client: %s:%d\n", inet_ntoa(clientsaddr[index].clientaddr.sin_addr), ntohs(clientsaddr[index].clientaddr.sin_port));
                write_stdout(msg);
            }
            else {
                write_stdout("Client overflow");
                continue;
            }            
        }

        for (int i = 0; i < client_count; i++) {
            if (!clients_equal(&recvaddr, &clientsaddr[i].clientaddr)) {
                if (send_sock(server_sockfd, &clientsaddr[i].clientaddr, (socklen_t) sizeof(clientsaddr[i].clientaddr), bufline) == -1) {
                    break;
                }
            }
        }
    }

    return NULL;
}