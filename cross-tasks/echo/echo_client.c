#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define LOG_SIZE        1000
#define MAX_BUF         65535
#define MAX_CLIENT      10
#define EXIT_MSG        "__EXIT__"

volatile int        running = 1;
int                 client_raw_sockfd = -1;
struct sockaddr_in  client_addr;
socklen_t           client_addr_len = sizeof(struct sockaddr_in);
struct sockaddr_in  server_addr;
socklen_t           server_addr_len = sizeof(struct sockaddr_in);
pthread_t           listen_thread;
pthread_t           send_thread;


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Parse port */
int port_pton(char* c_port, uint16_t* port);

/* Setup  socket settings */
int setup_socket(char* cclient_ip, char* cclient_port, char* cserver_ip, char* cserver_port);

/* Extract message from IP packet */
int get_server_msg_from_packet(char* msg, char* packet, uint16_t packet_len);

/* Insert message in IP packet */
void set_client_msg_in_packet(char* msg, char* packet, uint16_t packet_len);

/* Listen thread function */
void* receive_packet(void *arg);

/* Send thread function */
void* send_packet(void *arg);

int main(int argc, char* argv[])
{
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <client IP address> <client_port> <server IP address> <server port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((client_raw_sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    } 

    int on = 1;
    if (setsockopt(client_raw_sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (setup_socket(argv[1], argv[2], argv[3], argv[4]) == -1) {
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&listen_thread, NULL, receive_packet, NULL) != 0) {
        perror("pthread_create failed");
        close(client_raw_sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, send_packet, NULL) != 0) {
        perror("pthread_create failed");
        running = 0;
        pthread_cancel(listen_thread); 
        pthread_join(listen_thread, NULL);
        close(client_raw_sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_join(listen_thread, NULL);
    pthread_join(send_thread, NULL);

    close(client_raw_sockfd);
    exit(EXIT_SUCCESS);
}

void handle_sigint() {
    running = 0; 
    pthread_cancel(listen_thread);
    pthread_cancel(send_thread);  
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

int setup_socket(char* cclient_ip, char* cclient_port, char* cserver_ip, char* cserver_port) {
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, cclient_ip, &client_addr.sin_addr.s_addr) != 1) {
        perror("inet_pton failed");
        return -1;
    } 
    if (port_pton(cclient_port, &client_addr.sin_port) == -1) {
        perror("port_pton failed");
        return -1;
    }
    
    if (bind(client_raw_sockfd, (struct sockaddr*) &client_addr, (socklen_t) sizeof(struct sockaddr))) {
        perror("bind failed");
        return -1;
    }

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

    return 0;
}

void* receive_packet(void *arg) {
    char                msg[MAX_BUF];  
    char*               packet;
    uint16_t            packet_len;
    int                 recv_bytes = 0;
    struct sockaddr_in  recv_addr;
    socklen_t           recv_addr_len = sizeof(recv_addr);

    while (running) {  
        pthread_testcancel();

        /* Read ip packet length */
        uint16_t ip_length[2];
        int recv_bytes = recv(client_raw_sockfd, (char*) ip_length, 4, MSG_PEEK);
        packet_len = ntohs(ip_length[1]);

        /* Allocate memory for ip packet */
        packet = malloc(packet_len);
        if (packet == NULL) {
            perror("malloc failed");
            continue;
        }

        memset(msg, 0, sizeof(msg));
        memset(&recv_addr, 0, recv_addr_len);

        /* Read full ip packet */
        if ((recv_bytes = recvfrom(client_raw_sockfd, packet, packet_len, 0, (struct sockaddr*) &recv_addr, &recv_addr_len)) == -1) {
            free(packet);
            perror("recv failed");
            continue;
        }
        else {
            /* Get msg from ip packet */ 
            if (get_server_msg_from_packet(msg, packet, packet_len) == -1) {
                free(packet);
                continue;
            }
            write_stdout("< ");
            write_stdout(msg);
            write_stdout("\n");
        } 

        free(packet);
    }

    return NULL;
}

void* send_packet(void *arg) {
    char        msg[MAX_BUF];  
    char*       packet;
    uint16_t    packet_len;

    while (running) {      
        pthread_testcancel();

        /* Read msg from STDIN */
        write_stdout("> ");
        memset(msg, 0, sizeof(msg));
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = 0;

        if (strlen(msg) > 0) {

            /* Set ip packet length */
            packet_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(msg);
            /* Allocate memory for ip packet */
            packet = malloc(packet_len);
            if (packet == NULL) {
                perror("malloc failed");
                continue;
            }

            /* Configurate ip packet */
            set_client_msg_in_packet(msg, packet, packet_len);

            /* Send ip packet */
            if (sendto(client_raw_sockfd, packet, packet_len, 0,(struct sockaddr*) &server_addr, server_addr_len) == -1) {
                free(packet);
                perror("send failed");
                continue;
            }

            free(packet);
        }
    } 
    return NULL;  
}

int get_server_msg_from_packet(char* msg, char* packet, uint16_t packet_len) {
    struct iphdr* ip_header = (struct iphdr*) packet;
    struct udphdr* udp_header = (struct udphdr*) (packet + ip_header->ihl * 4);
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_len = packet_len - sizeof(struct udphdr) - sizeof(struct iphdr);

    if ((udp_header->dest != client_addr.sin_port) || (udp_header->source != server_addr.sin_port)) {
        return -1;
    }

    memcpy(msg, payload, payload_len);
    msg[payload_len] = '\0';
    
    return 0;
}

void set_client_msg_in_packet(char* msg, char* packet, uint16_t packet_len) {
    struct iphdr* ip_header = (struct iphdr*) packet;

    ip_header->version = 4;
    ip_header->ihl = 5;   

    struct udphdr* udp_header = (struct udphdr*) (packet + ip_header->ihl * 4);
    int payload_len = packet_len - sizeof(struct udphdr) - sizeof(struct iphdr);
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    strncpy(payload, msg, strlen(msg));

    ip_header->tot_len = htons(packet_len);
    ip_header->ttl = 64;
    ip_header->frag_off = 0;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->saddr = client_addr.sin_addr.s_addr;
    ip_header->daddr = server_addr.sin_addr.s_addr;
    ip_header->check = 0;

    udp_header->source = client_addr.sin_port;
    udp_header->dest = server_addr.sin_port;
    udp_header->len = htons(sizeof(struct udphdr) + payload_len);
    udp_header->check = 0;
}

