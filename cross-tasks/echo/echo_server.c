#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define LOG_SIZE        1000
#define MAX_CLIENT      10
#define EXIT_MSG        "__EXIT__"

typedef struct ClientInfo {
    struct sockaddr_in  client_addr;
    socklen_t           client_addr_len;
    int                 counter;
} ClientInfo;

volatile int        running = 1;
int                 server_raw_sockfd = -1;
struct sockaddr_in  server_addr;
socklen_t           server_addr_len = sizeof(struct sockaddr_in);
int                 clients_count = 0;
ClientInfo          clients_info[MAX_CLIENT];


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Parse port */
int port_pton(char* c_port, uint16_t* port);

/* Setup  socket settings */
int setup_socket(char* cserver_ip, char* cserver_port);

/* Add information about client in CLIENTS_INFO */
int add_client(ClientInfo* client_info);

/* Reset information about client session in CLIENTS_INFO */
int reset_client(ClientInfo* client_info);

/* Clear information about client session in CLIENTS_INFO */
int clear_client(ClientInfo* client_info) ;

/* Find client in CLIENTS_INFO */
int find_client(ClientInfo* client_info);

/* Deals with the processing and sending of messages */
void* processing_thread(void *arg); 

/* Receive client packet and parse it in CLIENT_INFO and CLIENT_MSG */
int receive_packet(ClientInfo* client_info, char** client_msg); 

/* Configurate server packet from CLIENT_INFO and SERVER_MSG and send it */
int send_packet(ClientInfo* client_info, char* server_msg); 

/* Parse IP packet, fill CLIENT_INFO and CLIENT_MSG */
int get_client_msg_from_packet(ClientInfo* client_info, char** client_msg, char* packet, uint16_t packet_len);

/* Configurate new IP packet from CLIENT_INFO and SERVER_MSG */
int set_server_msg_in_packet(ClientInfo* client_info, char* server_msg, char* packet, uint16_t packet_len);

/* Configurate SERVER_MSG from CLIENT_INFO and CLIENT_MSG */
int configurate_server_msg(ClientInfo* client_info, char* client_msg, char** server_msg);


int main(int argc, char* argv[])
{
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server IP address> <server port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((server_raw_sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int on = 1;
    if (setsockopt(server_raw_sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (setup_socket(argv[1], argv[2]) == -1) {
        exit(EXIT_FAILURE);
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, processing_thread, NULL) != 0) {
        perror("pthread_create failed");
        close(server_raw_sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_join(thread, NULL) != 0) {
        perror("pthread_join failed");
        close(server_raw_sockfd);
        exit(EXIT_FAILURE);
    }

    close(server_raw_sockfd);
    return EXIT_SUCCESS;
}

void handle_sigint() {
    running = 0;
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int port_pton(char* c_port, uint16_t* port) {  
    char *endptr;
    uint16_t p = strtol(c_port, &endptr, 10);
    if (*endptr != '\0' || p <= 0 || p > 65535) {
        return -1;
    }
    *port = htons((uint16_t)p);

    return 0;
}

int setup_socket(char* cserver_ip, char* cserver_port) {
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, cserver_ip, &server_addr.sin_addr.s_addr) != 1) {
        perror("inet_pton failed");
        return -1;
    } 
    if (port_pton(cserver_port, &server_addr.sin_port) == -1) {
        perror("port_pton failed");
        return -1;
    }
    
    if (bind(server_raw_sockfd , (struct sockaddr*) &server_addr, (socklen_t) sizeof(struct sockaddr))) {
        perror("bind failed");
        return -1;
    }

    return 0;
}

int add_client(ClientInfo* client_info) {
    if (clients_count == MAX_CLIENT) {
        return -1;
    }
    else {
        clients_info[clients_count].client_addr = client_info->client_addr;
        clients_info[clients_count].client_addr_len = sizeof(struct sockaddr_in);
        clients_info[clients_count].counter = 1;
        clients_count++;
    }  
    
    return 0;
}

int reset_client(ClientInfo* client_info) {
    int idx = find_client(client_info);
    if (idx == -1){
        return -1;
    }
    else {
        clients_info[idx].counter++;
        client_info->counter = clients_info[idx].counter;
    }

    return 0;
}

int clear_client(ClientInfo* client_info) {
    int idx = find_client(client_info);
    if (idx == -1){
        return -1;
    }
    else {
        clients_info[idx].counter = 0;
    }

    return 0;
}

int find_client(ClientInfo* client_info) {
    for (int i = 0; i < clients_count; i++) {
        if (clients_info[i].client_addr.sin_addr.s_addr == client_info->client_addr.sin_addr.s_addr
            && clients_info[i].client_addr.sin_port == client_info->client_addr.sin_port) {
            return i;
        }
    }
    return -1;
}

void *processing_thread(void *arg) {
    ClientInfo  client_info;
    char*       client_msg;
    char*       server_msg;
    
    while (running) {
        pthread_testcancel();

        memset(&client_info, 0, sizeof(ClientInfo));
        client_msg = NULL;
        server_msg = NULL;

        if (receive_packet(&client_info, &client_msg) == -1) {
            continue;
        }

        if (strcmp(client_msg, EXIT_MSG) == 0) {
            if (clear_client(&client_info) != -1) {
                char log_msg[LOG_SIZE];
                sprintf(log_msg, "Client %s:%d disconnected\n", inet_ntoa(client_info.client_addr.sin_addr), ntohs(client_info.client_addr.sin_port));
                write_stdout(log_msg);
            }
            if (client_msg != NULL) {
                free(client_msg);
            }
            continue;
        }
        else {
            if (reset_client(&client_info) == -1) {
                add_client(&client_info);

                char log_msg[LOG_SIZE];
                sprintf(log_msg, "Client %s:%d connected\n", inet_ntoa(client_info.client_addr.sin_addr), ntohs(client_info.client_addr.sin_port));
                write_stdout(log_msg);
            }
        }

        if (configurate_server_msg(&client_info, client_msg, &server_msg) == -1) {
            if (client_msg != NULL) {
                free(client_msg);
            }
            continue;
        } 


        if (send_packet(&client_info, server_msg) != -1) {
            char log_msg[LOG_SIZE];
            sprintf(log_msg, "Send to %s:%d\n", inet_ntoa(client_info.client_addr.sin_addr), ntohs(client_info.client_addr.sin_port));
            write_stdout(log_msg);
        }
        
        if (client_msg != NULL) {
            free(client_msg);
        }
        if (server_msg != NULL) {
            free(server_msg);
        }
    }

    return NULL;
}

int receive_packet(ClientInfo* client_info, char** client_msg) {
    char*       packet;
    uint16_t    packet_len;

    /* Read ip packet length */
    uint16_t ip_length[2];
    int recv_bytes = recv(server_raw_sockfd, (char*) ip_length, 4, MSG_PEEK);
    packet_len = ntohs(ip_length[1]);

    /* Allocate memory for ip packet */
    packet = malloc(packet_len);
    if (packet == NULL) {
        perror("malloc failed");
        return -1;
    }

    /* Read full ip packet */
    if ((recv_bytes = recvfrom(server_raw_sockfd, packet, packet_len, 0, (struct sockaddr*) &client_info->client_addr, &client_info->client_addr_len)) == -1) {
        perror("recv failed");
        free(packet);
        return -1;
    }


    /* Get msg from ip packet */ 
    if (get_client_msg_from_packet(client_info, client_msg, packet, packet_len) == -1) {
        free(packet);
        return -1;
    }
     
    free(packet);
    return 0;
}

int send_packet(ClientInfo* client_info, char* server_msg) {
    char*       packet;
    uint16_t    packet_len;

    /* Set ip packet length */
    packet_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(server_msg);
    /* Allocate memory for ip packet */
    packet = malloc(packet_len);
    if (packet == NULL) {
        perror("malloc failed");
        return -1;
    }

    /* Configurate ip packet */
    set_server_msg_in_packet(client_info, server_msg, packet, packet_len);

    /* Send ip packet */
    if (sendto(server_raw_sockfd, packet, packet_len, 0, (struct sockaddr*) &client_info->client_addr, client_info->client_addr_len) == -1) {
        perror("send failed");
        free(packet);
        return -1;
    }

    free(packet);
    return 0;
}

int get_client_msg_from_packet(ClientInfo* client_info, char** client_msg, char* packet, uint16_t packet_len) {
    struct iphdr* ip_header = (struct iphdr*) packet;
    struct udphdr* udp_header = (struct udphdr*) (packet + ip_header->ihl * 4);
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_len = packet_len - sizeof(struct udphdr) - sizeof(struct iphdr);

    if (udp_header->dest != server_addr.sin_port) {
        return -1;
    }

    client_info->client_addr.sin_addr.s_addr = ip_header->saddr;
    client_info->client_addr.sin_port = udp_header->source;
    client_info->client_addr.sin_family = AF_INET;
    client_info->counter = 1;

    *client_msg = malloc(payload_len + 1);
    if (*client_msg == NULL) {
        perror("malloc failed");
        return -1;
    }
    memcpy(*client_msg, payload, payload_len);
    (*client_msg)[payload_len] = '\0';
    
    return 0;
}

int set_server_msg_in_packet(ClientInfo* client_info, char* server_msg, char* packet, uint16_t packet_len) {
    struct iphdr* ip_header = (struct iphdr*) packet;

    ip_header->ihl = 5;
    ip_header->version = 4;

    struct udphdr* udp_header = (struct udphdr*) (packet + ip_header->ihl * 4);
    int payload_len = packet_len - sizeof(struct udphdr) - sizeof(struct iphdr);
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    strncpy(payload, server_msg, strlen(server_msg));

    ip_header->tot_len = htons(packet_len);
    ip_header->ttl = 64;
    ip_header->frag_off = 0;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->saddr = server_addr.sin_addr.s_addr;
    ip_header->daddr = client_info->client_addr.sin_addr.s_addr;
    ip_header->check = 0;

    udp_header->source = server_addr.sin_port;
    udp_header->dest = client_info->client_addr.sin_port;
    udp_header->len = htons(sizeof(struct udphdr) + payload_len);
    udp_header->check = 0;

    return 0;
}

int configurate_server_msg(ClientInfo* client_info, char* client_msg, char** server_msg) {
    if (client_info == NULL || client_msg == NULL) {
        return -1;
    }

    int msg_len = strlen(client_msg);
    int buffer_size = msg_len + 3; 
    *server_msg = malloc(buffer_size);

    if (*server_msg == NULL) {
        perror("malloc failed");
        return -1;
    }

    snprintf(*server_msg, buffer_size, "%s %d", client_msg, client_info->counter);

    return 0;
}