#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUF 1024

int     running = 1;

/* SIGINT handle */
void handle_sigint(); 

/* Write to STDOUT */
void write_stdout(const char *str);

/* Parse port number */
uint16_t parse_port(const char *port_str);

/* Connect to the server */
int connect_to_server(int sockfd, const char *ip, const char *port);

/* Communication loop */
int communication_loop(int sockfd);


int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server IP address> <server port>\n", argv[0]);
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

int communication_loop(int client_sockfd) {
    char bufline[MAX_BUF];
    int bytesread;

    while ((bytesread = recv(client_sockfd, bufline, sizeof(bufline) - 1, 0)) > 0 && running) {
        bufline[bytesread] = '\0';
        
        char msg[2 * MAX_BUF];
        snprintf(msg, sizeof(msg), "< (Server) %s", bufline);
        write_stdout(msg);

        if (fgets(bufline, sizeof(bufline), stdin) == NULL) {
            perror("fgets failed");
            return -1;
        }

        if (strcmp(bufline, "quit\n") == 0) {
            write_stdout("Exit...\n");
            break;
        }

        if (send(client_sockfd, bufline, strlen(bufline), 0) == -1) {
            perror("send failed");
            return -1;
        }
    }

    if (bytesread == -1) {
        perror("recv failed");
        return -1;
    }

    return 0;
}
