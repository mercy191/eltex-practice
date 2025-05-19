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
#define MAX_UDP_PAYLOAD 1472
#define MAX_IP_LENGHT   1500
#define MAX_CLIENT      10
#define EXIT_MSG        "__EXIT__"

volatile int        running = 1;
int                 client_raw_sockfd = -1;
uint32_t            client_ip = 0;
uint16_t            client_port = 0;
uint32_t            server_ip = 0;
uint16_t            server_port = 0;
pthread_t           listen_thread;
pthread_t           send_thread;


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Setup IP port settings */
int setup(char* cclient_ip, char* cclient_port, char* cserver_ip, char* cserver_port);

/* Extract message from IP packet */
int get_src_msg_from_packet(char* msg, char* packet);

/* Insert message in IP packet */
void set_dest_msg_in_packet(char* msg, char* packet) ;

/* Listen thread function */
void* listening(void *arg);

/* Send thread function */
void* sending(void *arg);

/* Listen socket */
int listen_sock(int sockfd, char *bufline, int buflen);

/* Send message in socket */
int send_sock(int sockfd, char *bufline, int buflen);


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

    if (setup(argv[1], argv[2], argv[3], argv[4]) == -1) {
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

    if (pthread_create(&listen_thread, NULL, listening, NULL) != 0) {
        perror("pthread_create failed");
        close(client_raw_sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, sending, NULL) != 0) {
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
 
int setup(char* cclient_ip, char* cclient_port, char* cserver_ip, char* cserver_port) {
    struct in_addr client_addr;
    if (inet_pton(AF_INET, cclient_ip, &client_addr) == 1) {
        client_ip = client_addr.s_addr;
    } 
    else {
        perror("inet_pton failed");
        return -1;
    }

    struct in_addr server_addr;
    if (inet_pton(AF_INET, cserver_ip, &server_addr) == 1) {
        server_ip = server_addr.s_addr;
    } 
    else {
        perror("inet_pton failed");
        return -1;
    }

    client_port = htons(strtol(cclient_port, NULL, 10));
    server_port = htons(strtol(cserver_port, NULL, 10));

    return 0;
}

int get_src_msg_from_packet(char* msg, char* packet) {
    struct iphdr* ip_header = (struct iphdr*) packet;
    struct udphdr* udp_header = (struct udphdr*) (ip_header + sizeof(struct iphdr));
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_len = MAX_UDP_PAYLOAD;

    if (udp_header->dest != client_port) {
        return -1;
    }

    strncpy(msg, payload, payload_len);
    
    return 0;
}

void set_dest_msg_in_packet(char* msg, char* packet) {
    struct iphdr* ip_header = (struct iphdr*) packet;
    struct udphdr* udp_header = (struct udphdr*)(packet + sizeof(struct iphdr));
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_len = MAX_UDP_PAYLOAD;

    strncpy(payload, msg, strlen(msg));

    ip_header->ihl = 5;
    ip_header->version = 4;
    ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_len);
    ip_header->ttl = 64;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->saddr = client_ip;
    ip_header->daddr = server_ip;

    udp_header->source = client_port;
    udp_header->dest = server_port;
    udp_header->len = htons(sizeof(struct udphdr) + payload_len);


    printf("IP header size: %ld\n", sizeof(struct iphdr));
    printf("IP total length: %u\n", ntohs(ip_header->tot_len));
    printf("IP source address: %u\n", ntohl(client_ip));
    printf("IP dest address: %u\n", ntohl(server_ip));

    printf("UDP header size: %ld\n", sizeof(struct udphdr));
    printf("UDP source port: %u\n", ntohs(client_port));
    printf("UDP dest port: %u\n", ntohs(server_port));
    printf("UDP total length: %u\n", ntohs(udp_header->len));
    printf("UDP payload size: %d\n", payload_len);

    printf("Msg size: %ld\n", strlen(msg));
}

void* listening(void *arg) {
    char msg[MAX_UDP_PAYLOAD];  
    char packet[MAX_IP_LENGHT];

    while (running) {  
        pthread_testcancel();

        memset(msg, 0, sizeof(msg));
        memset(packet, 0, MAX_IP_LENGHT);
        if (listen_sock(client_raw_sockfd, packet, MAX_IP_LENGHT) < 0) {
            continue;
        }
        else {
            if (get_src_msg_from_packet(msg, packet) == -1) {
                continue;
            }
            write_stdout(msg);
            write_stdout("\n");
        } 
    }

    return NULL;
}

void* sending(void *arg) {
    char msg[MAX_UDP_PAYLOAD];  
    char packet[MAX_IP_LENGHT];

    while (running) {      
        pthread_testcancel();

        memset(msg, 0, sizeof(msg));
        memset(packet, 0, MAX_UDP_PAYLOAD);
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = 0;

        if (strlen(msg) > 0) {
            set_dest_msg_in_packet(msg, packet);

            if (send_sock(client_raw_sockfd, packet, MAX_IP_LENGHT) < 0) {
                continue;
            }
        }
    } 
    return NULL;  
}

int listen_sock(int sockfd, char *bufline, int buflen) {         
    int bytesread = 0;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if ((bytesread = recvfrom(sockfd, bufline, buflen, 0, (struct sockaddr*) &addr, &addrlen)) == -1) {
        perror("recvfrom failed");
        return -1;
    }
    else if (bytesread == 0) {
        return 0;
    }

    return bytesread;
}

int send_sock(int sockfd, char *bufline, int buflen) {
    struct sockaddr_in  addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = server_ip;
    addr.sin_port = server_port;
    socklen_t addrlen = sizeof(addr);

    if (sendto(sockfd, bufline, buflen, 0, (struct sockaddr*) &addr, addrlen) == -1) {
        perror("sendto failed");
        return -1;
    }

    return 0;
}