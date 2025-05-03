#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

#define MAX_BUF 1024

static volatile int running = 1;

/* SIGINT handle */
void handle_sigint(); 

/* Write to STDOUT */
void write_stdout(const char *str);

/* Parse port number */
uint16_t parse_port(const char *port_str);

/* Connect to the server */
int connect_to_server(int sockfd, const char *ip, const char *port);

/* Math calculation function */
int math_caluculation(int client_sockfd);

/* Send file to server */
int file_sending(int client_sockfd, char* filename);

/* Communication loop */
int communication_loop(int sockfd);


int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server IP address> <server port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd == -1) {
        exit(EXIT_FAILURE);
    }

    if (connect_to_server(client_sockfd, argv[1], argv[2]) == -1) {
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    if (communication_loop(client_sockfd) == -1) {
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    close(client_sockfd);
    return EXIT_SUCCESS;
}

void handle_sigint() {
    running = 0;
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

uint16_t parse_port(const char *port_str) {
    char *endptr;
    long port = strtol(port_str, &endptr, 10);
    if (*endptr != '\0' || port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %s\n", port_str);
        exit(EXIT_FAILURE);
    }

    return (uint16_t)port;
}

int connect_to_server(int client_sockfd, const char *server_ip, const char *server_port) {

    uint16_t serverport = parse_port(server_port);
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(serverport);

    if (inet_aton(server_ip, &serveraddr.sin_addr) == 0) {
        perror("Invalid IP address");
        return -1;
    }

    if (connect(client_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("connect failed");
        return -1;
    }

    return 0;
}

int math_caluculation(int client_sockfd) {
    int     bytesread = 0;
    char    bufline[MAX_BUF];
    int     buflen = sizeof(bufline);

    if (send(client_sockfd, "calc", strlen("calc"), 0) == -1) {
        perror("send failed");
        return -1;
    }

    for (int i = 0; i < 3; ++i) {
        memset(bufline, 0, buflen);
        if ((bytesread = recv(client_sockfd, bufline, buflen - 1, 0)) <= 0) {
            perror("recv failed");
            return -1;
        }
        bufline[bytesread] = 0;
        write_stdout(bufline);

        if (fgets(bufline, buflen, stdin) == NULL) {
            perror("fgets failed");
            return -1;
        }

        if (send(client_sockfd, bufline, strlen(bufline), 0) == -1) {
            perror("send failed");
            return -1;
        }
    }

    memset(bufline, 0, buflen);
    if ((bytesread = recv(client_sockfd, bufline, buflen - 1, 0)) <= 0) {
        perror("recv failed");
        return -1;
    }
    bufline[bytesread] = 0;
    write_stdout(bufline);

    return 0;
}

int file_sending(int client_sockfd, char* filename) {
    int     bytesread = 0;
    char    bufline[MAX_BUF];
    int     buflen = sizeof(bufline);

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen failed");
        return 0;
    }    

    if (send(client_sockfd, "sendfile", strlen("sendfile"), 0) == -1) {
        perror("send failed");
        return -1;
    }
    sleep(1);
    
    if (send(client_sockfd, filename, strlen(filename), 0) == -1) {
        perror("send failed");
        return -1;
    }
    sleep(1);

    memset(bufline, 0, buflen);
    while ((bytesread = fread(bufline, 1, buflen - 1, fp)) > 0) {
        if (send(client_sockfd, bufline, bytesread, 0) == -1) {
            perror("send failed");
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    sleep(1);

    if (send(client_sockfd, "EOF", 3, 0) == -1) {
        perror("send failed");
        return -1;
    }
    write_stdout("> File sent successfully.\n");

    return 0;
}

int communication_loop(int client_sockfd) {
    int     bytesread = 0;
    char    bufline[MAX_BUF];
    int     buflen = sizeof(bufline);

    while (running) {
   
        write_stdout("< Enter command (calc / sendfile <filename> / quit):\n");

        memset(bufline, 0, buflen);
        if (fgets(bufline, buflen, stdin) == NULL) {
            perror("fgets failed");
            return -1;
        }
        bufline[strcspn(bufline, "\r\n")] = 0;

        if (strncmp(bufline, "calc", 4) == 0) {
            if (math_caluculation(client_sockfd) == -1) {
                break;
            }
        }
        else if (strncmp(bufline, "sendfile ", 9) == 0) {
            char* filename = bufline + 9;
            if (file_sending(client_sockfd, filename) == -1) {
                break;
            }
        }
        else if (strncmp(bufline, "quit", 4) == 0) {
            write_stdout("> Exit...\n");
            break;
        }
        else {
            write_stdout("> Unknown command.\n");
        }
    } 

    return 0;
}
