#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>

#define MAX_BUF             1024
#define MAX_CONNECTIONS     5
#define WAIT_TIME           5

#define MSG_FIRST_PARAM     "< (Server) Enter 1 parameter\r\n"
#define MSG_SECOND_PARAM    "< (Server) Enter 2 parameter\r\n"
#define MSG_FUNC_PARAM      "< (Server) Enter function\r\n"

typedef int (*operation_func)(double*, int, int);

typedef struct {
    const char* name;
    operation_func func;
} operation_t;

/* Math operations */
int add(double* res, int a, int b);
int subtract(double* res, int a, int b);
int multiply(double* res, int a, int b);
int divide(double* res, int a, int b);

/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char* str);

/* Parse port number */
uint16_t parse_port(const char *port_str);

/* Setup socket setting */
int setup_server_sock(int sockfd, uint16_t port);

/* Get parameter from client */
int get_param(int client_sockfd, char* bufline, int buflen, const char* prompt);

/* Select and call operation */
void call_selected_operation(const char* operation_name, int a, int b, char* response, int response_len);

/* Math calculation function */
int math_caluculation(int client_sockfd);

/* Receive file from client */
int file_receive(int client_sockfd);

/* Accept new connection to server */
void accept_new_connection(int server_sockfd, int* client_sockets, int* max_fd);

/* Processes user interaction */
int communication_process(int* client_sockets, fd_set* readfds);

/* Main server cycle */
void server_loop(int server_sockfd);

/* Operations table */
operation_t operations[] = {
    {"add", add},
    {"subtract", subtract},
    {"multiply", multiply},
    {"divide", divide}
};
const int   operations_count = sizeof(operations) / sizeof(operations[0]);
int         running = 1;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int server_port = parse_port(argv[1]);
    
    if (setup_server_sock(server_sockfd, server_port) == -1) {
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    server_loop(server_sockfd);

    close(server_sockfd);
    return 0;
}

void handle_sigint() {
    running = 0;
}

void write_stdout(const char* str) {
    write(STDOUT_FILENO, str, strlen(str));
}

uint16_t parse_port(const char *port_str) {
    char *endptr;
    long port = strtol(port_str, &endptr, 10);

    return (uint16_t)port;
}

int setup_server_sock(int sockfd, uint16_t port) {
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return -1;
    }

    if (listen(sockfd, MAX_CONNECTIONS) < 0) {
        perror("listen failed");
        return -1;
    }

    return 0;
}

int get_param(int client_sockfd, char* bufline, int buflen, const char* prompt) {
    if (send(client_sockfd, prompt, strlen(prompt), 0) < 0) {
        perror("send failed");
        return -1;
    }

    memset(bufline, 0, buflen);
    int bytesread = recv(client_sockfd, bufline, buflen - 1, 0);
    if (bytesread == -1) {
        perror("read failed");
        return -1;
    }
    else if (bytesread == 0) {
        return 1;
    }

    bufline[strcspn(bufline, "\r\n")] = 0; 
    return 0;
}

void call_selected_operation(const char* operation_name, int a, int b, char* response, int response_len) {
    double result;
    for (int i = 0; i < operations_count; ++i) {
        if (strncmp(operations[i].name, operation_name, strlen(operation_name)) == 0) {
            if (operations[i].func(&result, a, b) == 0) {
                snprintf(response, response_len, "%lf\n", result);
            } 
            else {
                snprintf(response, response_len, "Error: division by zero\n");
            }
            return;
        }
    }
    
    snprintf(response, response_len, "Undefined operation\n");
    return;
}

int math_caluculation(int client_sockfd) {
    int     a, b;
    char    bufline[MAX_BUF];
    int     buflen = sizeof(bufline);  

    if (get_param(client_sockfd, bufline, buflen, MSG_FIRST_PARAM) != 0) {
        return -1;
    }
    a = atoi(bufline);

    if (get_param(client_sockfd, bufline, buflen, MSG_SECOND_PARAM) != 0) {
        return -1;
    }
    b = atoi(bufline);

    if (get_param(client_sockfd, bufline, buflen, MSG_FUNC_PARAM) != 0) {
        return -1;
    }

    char response[MAX_BUF];
    call_selected_operation(bufline, a, b, response, sizeof(response));

    if (send(client_sockfd, response, strlen(response), 0) < 0) {
        perror("send failed");
        return -1;
    }

    return 0;
}

int file_receive(int client_sockfd) {
    int     bytesread = 0;
    char    bufline[MAX_BUF];
    int     buflen = sizeof(bufline);

    if (recv(client_sockfd, bufline, buflen - 1, 0) <= 0) {
        perror("recv failed");
        return -1;
    }

    bufline[strcspn(bufline, "\r\n")] = 0;
    char filename[MAX_BUF];
    strncpy(filename, bufline, sizeof(filename));

    write_stdout("> Receiving file: ");
    write_stdout(filename);
    write_stdout("\n");
    
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen failed");
        return -1;
    }

    memset(bufline, 0, buflen);
    while ((bytesread = recv(client_sockfd, bufline, buflen - 1, 0)) > 0) {
        if (bytesread == 3 && strncmp(bufline, "EOF", 3) == 0) {
            break;
        }
        fwrite(bufline, 1, bytesread, fp);
    }
    fclose(fp);

    write_stdout("> File received successfully.\n");
    return 0;
}

void accept_new_connection(int server_sockfd, int* client_sockets, int* max_fd) {
    struct sockaddr_in  client_addr;
    socklen_t           client_len = sizeof(client_addr);

    int client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sockfd == -1) {
        perror("accept failed");
        return;
    }

    struct hostent* hst = gethostbyaddr((char*)&client_addr.sin_addr, sizeof(client_addr.sin_addr), AF_INET);
    char msg[MAX_BUF];
    snprintf(msg, sizeof(msg), "+ %s [%s] connected\n", hst ? hst->h_name : "Unknown host", inet_ntoa(client_addr.sin_addr));
    write_stdout(msg);

    int slot_found = 0;
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = client_sockfd;
            if (client_sockfd > *max_fd) {
                *max_fd = client_sockfd;
            }
            slot_found = 1;
            break;
        }
    }

    if (!slot_found) {
        const char* full_msg = "Server busy. Try again later.\n";
        send(client_sockfd, full_msg, strlen(full_msg), 0);
        close(client_sockfd);
        write_stdout("! Rejected connection: too many clients\n");
    }
}

int communication_process(int* client_sockets, fd_set* readfds) {
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        int client_sockfd = client_sockets[i];
        if (client_sockfd > 0 && FD_ISSET(client_sockfd, readfds)) {
            char bufline[MAX_BUF];
            int bytesread = recv(client_sockfd, bufline, sizeof(bufline) - 1, 0);
            if (bytesread <= 0) {
                close(client_sockfd);
                client_sockets[i] = 0;
                write_stdout("- disconnect\n");
                continue;
            }

            bufline[bytesread] = '\0';

            if (strcmp(bufline, "calc") == 0) {
                if (math_caluculation(client_sockfd) == -1) {
                    break;
                }
            } 
            else if (strcmp(bufline, "sendfile") == 0) {
                if (file_receive(client_sockfd) == -1) {
                    break;
                }
            } 
            else if (strcmp(bufline, "quit") == 0) {
                break;
            } 
            else {
                write_stdout("> Unknown command received.\n");
            }
        }
    }

    return 0;
}

void server_loop(int server_sockfd) {
    fd_set readfds;
    int max_fd = server_sockfd;
    int client_sockets[MAX_CONNECTIONS] = {0};

    while (running) {
        FD_ZERO(&readfds);
        FD_SET(server_sockfd, &readfds);

        for (int i = 0; i < MAX_CONNECTIONS; ++i) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
            }
        }

        int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ret == -1) {
            if (errno == EINTR && !running) break;
            else continue;
        }

        if (FD_ISSET(server_sockfd, &readfds)) {
            accept_new_connection(server_sockfd, client_sockets, &max_fd);
        }

        communication_process(client_sockets, &readfds);
    }
}

int add(double* res, int a, int b) {
    *res = a + b;
    return 0;
}

int subtract(double* res, int a, int b) {
    *res = a - b;
    return 0;
}

int multiply(double* res, int a, int b) {
    *res = a * b;
    return 0;
}

int divide(double* res, int a, int b) {
    if (b == 0) return -1;
    *res = (double)a / b;
    return 0;
}
