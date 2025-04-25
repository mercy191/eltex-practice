#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define MAX_BUF     1000
#define CLI_COUNT   3

struct socksaddr_in {
    struct sockaddr_in clientaddr;
    int active;
};

volatile int            running = 1;
char                    bufline[MAX_BUF];        
int                     client_count = 0;
int                     sockfd;
struct sockaddr_in      serveraddr;
struct socksaddr_in     clientsaddr[CLI_COUNT] = {0};
uint16_t                servport = 0;


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Comprassion of clients IP adresses and ports */
int clients_equal(struct sockaddr_in *a, struct sockaddr_in *b);


int main(int argc, char* argv[]) 
{
    signal(SIGINT, handle_sigint);

    if (argc != 2) {
        perror("Usage: udp_server <server port>\n");
        exit(EXIT_FAILURE);
    }

    servport = (uint16_t)strtol(argv[1], NULL, 10);

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket failed"); 
        exit(EXIT_FAILURE);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(servport);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in recvaddr;
    socklen_t recvaddrlen = sizeof(recvaddr);

    /* Process one message at a time */
    while (running) {
        memset(bufline, 0, MAX_BUF);      
        if (recvfrom(sockfd, bufline, sizeof(MAX_BUF), 0, (struct sockaddr *) &recvaddr, &recvaddrlen) == -1) {
            perror("recvfrom failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (!running) break;

        int known = 0;
        for (int i = 0; i < CLI_COUNT; i++) {
            if (clientsaddr[i].active && clients_equal(&recvaddr, &clientsaddr[i].clientaddr)) {
                known = 1;
                break;
            }
        }
        if (!known && client_count < CLI_COUNT) {
            for (int i = 0; i < CLI_COUNT; i++) {
                if (!clientsaddr[i].active) {
                    memcpy(&clientsaddr[client_count].clientaddr, &recvaddr, sizeof(recvaddr));
                    clientsaddr[client_count].active = 1;
                    char msg[MAX_BUF];
                    snprintf(msg, sizeof(msg), "New client: %s:%d\n", inet_ntoa(clientsaddr[client_count].clientaddr.sin_addr), ntohs(clientsaddr[client_count].clientaddr.sin_port));
                    write_stdout(msg);
                    client_count++;
                    break;
                }
            }           
        }
        else if (!known && client_count >= CLI_COUNT) {
            char msg[MAX_BUF];
            snprintf(msg, sizeof(msg), "Client overflow: %s:%d\n", inet_ntoa(clientsaddr[client_count].clientaddr.sin_addr), ntohs(clientsaddr[client_count].clientaddr.sin_port));
            write_stdout(msg);
            continue;
        }

        for (int i = 0; i < client_count; i++) {
            if (!clients_equal(&recvaddr, &clientsaddr[i].clientaddr)) {
                if (sendto(sockfd, bufline, MAX_BUF, 0, (struct sockaddr*) &clientsaddr[i].clientaddr, (socklen_t) sizeof(clientsaddr[i].clientaddr)) == -1) {
                    perror("sendto failed");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
}

void handle_sigint() {
    running = 0; 

    struct sockaddr_in stopaddr;
    stopaddr.sin_family = AF_INET;
    stopaddr.sin_port = serveraddr.sin_port;
    stopaddr.sin_addr.s_addr = serveraddr.sin_addr.s_addr;
    sendto(sockfd, "exit", 4, 0, (struct sockaddr *) &stopaddr, (socklen_t) sizeof(stopaddr));
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int clients_equal(struct sockaddr_in *a, struct sockaddr_in *b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}