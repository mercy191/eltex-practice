#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
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

/* Send SIGINT to child process*/
void send_sigint_to_children(pid_t parent_pid);

/* Write in STDOUT */
void write_stdout(const char* str);

/* Parse port number */
uint16_t parse_port(const char *port_str);

/* Setup socket setting */
int setup_server_sock(int server_sockfd, uint16_t port);

/* Get parameter from client */
int get_param(int client_sockfd, char* bufline, int buflen, const char* prompt);

/* Select and call operation */
void call_selected_operation(const char* operation_name, int a, int b, char* response, int response_len);

/* Math calculation function */
int math_caluculation(int client_sockfd);

/* Processes user interaction */
int communication_process(int client_sockfd);

/* Таблица операций */
operation_t operations[] = {
    {"add", add},
    {"subtract", subtract},
    {"multiply", multiply},
    {"divide", divide}
};
const int           operations_count = sizeof(operations) / sizeof(operations[0]);
static volatile int running = 1;

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

    fd_set readfds;
    while (running) {
        FD_ZERO(&readfds);
        FD_SET(server_sockfd, &readfds);

        int ret = select(server_sockfd + 1, &readfds, NULL, NULL, NULL);
        if (ret == -1) {
            if (errno == EINTR && !running) {
                break;
            }
            else {
                continue;
            }
            perror("select failed");
            break;
        }

        if (FD_ISSET(server_sockfd, &readfds)) {
            struct sockaddr_in  client_addr;
            socklen_t           client_len = sizeof(client_addr);
    
            int client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addr, &client_len);
            if (client_sockfd == -1) {
                perror("accept failed");
                continue;
            }
    
            struct hostent* hst = gethostbyaddr((char*)&client_addr.sin_addr, sizeof(client_addr.sin_addr), AF_INET);
            char msg[MAX_BUF];
            snprintf(msg, sizeof(msg), "+ %s [%s] connected\n", hst ? hst->h_name : "Unknown host", inet_ntoa(client_addr.sin_addr));
            write_stdout(msg);
    
            /* Create child process*/
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                close(client_sockfd);
                continue;
            }
            /* Child process */
            else if (pid == 0) { 
                close(server_sockfd);
                communication_process(client_sockfd);
                close(client_sockfd);
                exit(EXIT_SUCCESS);
            }
            /* Parent process */
            else { 
                close(client_sockfd);
            }
        }       
    }
    send_sigint_to_children(getpid());
    while (wait(NULL) > 0);
    close(server_sockfd);
    exit(EXIT_SUCCESS);
}

void handle_sigint() {
    running = 0;
}

void send_sigint_to_children(pid_t parent_pid) {
    DIR *dir;
    struct dirent *entry;
    char path[MAX_BUF];
    FILE *fp;
    char buf[MAX_BUF];
    pid_t pid, ppid;

    dir = opendir("/proc");
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') 
            continue;

        pid = atoi(entry->d_name);  
        if (pid == 0) continue;

        snprintf(path, sizeof(path), "/proc/%d/stat", pid);

        fp = fopen(path, "r");
        if (fp == NULL) {
            continue;
        }

        if (fgets(buf, sizeof(buf), fp) != NULL) {
            sscanf(buf, "%d %*s %*c %d", &pid, &ppid);
            if (ppid == parent_pid) {
                if (kill(pid, SIGINT) == -1) {
                    perror("kill failed");
                }
            }
        }
        fclose(fp);
    }
    closedir(dir);
}

void write_stdout(const char* str) {
    write(STDOUT_FILENO, str, strlen(str));
}

uint16_t parse_port(const char *port_str) {
    char *endptr;
    long port = strtol(port_str, &endptr, 10);

    return (uint16_t)port;
}

int setup_server_sock(int client_sockfd, uint16_t port) {
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(client_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return -1;
    }

    if (listen(client_sockfd, MAX_CONNECTIONS) < 0) {
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
    int bytesread = recv(client_sockfd, bufline, buflen, 0);
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
    int a, b;
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

int communication_process(int client_sockfd) {
    int     bytesread = 0;
    char    bufline[MAX_BUF];
    int     buflen = sizeof(bufline);
    fd_set  readfds; 

    while (running) {
        FD_ZERO(&readfds);
        FD_SET(client_sockfd, &readfds);

        int ret = select(client_sockfd + 1, &readfds, NULL, NULL, NULL);     

        if (ret > 0 && FD_ISSET(client_sockfd, &readfds)) {           
            memset(bufline, 0, sizeof(bufline));
            if ((bytesread = recv(client_sockfd, bufline, sizeof(bufline) - 1, 0)) <= 0) {
                break;
            }
            bufline[strcspn(bufline, "\r\n")] = 0;

            if (strcmp(bufline, "calc") == 0) {
                if (math_caluculation(client_sockfd) == -1){
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

    write_stdout("- disconnect\n");
    return 0;
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
