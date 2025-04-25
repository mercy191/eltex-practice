#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define MAX_BUF 1000

volatile int        running = 1;
char                bufline[MAX_BUF];  
int                 sockfd;
struct sockaddr_in  serveraddr;
struct sockaddr_in  clientaddr;
uint16_t            servport = 0;


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Listen socket */
int listen_sock();

/* Send message in socket */
int send_sock();

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_sigint);

    if (argc != 3){
        perror("Usage: udp_client <server IP address> <server port>\n");
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
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    /* Child process */
    else if (pid == 0) {
        int result = listen_sock();
        exit(result);
    }
    /* Parent process */
    else {       
        int result = send_sock();

        /* Send SIGINT */
        if (kill(pid, SIGINT) == -1) {
            perror("kill failed");
            exit(EXIT_FAILURE);
        }
    
        close(sockfd);
        exit(result);
    }
}

void handle_sigint() {
    running = 0; 

    struct sockaddr_in stopaddr;
    stopaddr.sin_family = AF_INET;
    stopaddr.sin_port = clientaddr.sin_port;
    stopaddr.sin_addr.s_addr = clientaddr.sin_addr.s_addr;
    sendto(sockfd, "exit", 4, 0, (struct sockaddr *) &stopaddr, (socklen_t) sizeof(stopaddr));
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int listen_sock() {
    while (running) {
        memset(bufline, 0, sizeof(bufline));
        fgets(bufline, MAX_BUF, stdin);

        if (sendto(sockfd, bufline, strlen(bufline) + 1, 0, (struct sockaddr*) &serveraddr, (socklen_t) sizeof(serveraddr)) == -1) {
            perror("sendto failed");
            close(sockfd);
            return EXIT_FAILURE;
        }
    } 
    return EXIT_SUCCESS;  
}

int send_sock() {
    struct sockaddr_in recvaddr;
    socklen_t recvaddrlen = sizeof(recvaddr);

    while (running) {  
         memset(bufline, 0, sizeof(bufline));
        if (recvfrom(sockfd, bufline, MAX_BUF, 0, (struct sockaddr*) &recvaddr, &recvaddrlen) == -1) {
            perror("recvfrom failed");
            close(sockfd);
            return EXIT_FAILURE;
        }
    
        if (!running) break;
    
        char msg[2*MAX_BUF];
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &recvaddr.sin_addr, ip_str, INET_ADDRSTRLEN);
        snprintf(msg, sizeof(msg), "< (%s) %s", ip_str, bufline);
        write_stdout(msg);
    }

    return EXIT_SUCCESS;
}