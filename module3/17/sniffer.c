#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>  
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define MAX_BUF 1000

volatile int    running = 1;
int             raw_sockfd = -1;

/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Parse port number */
uint16_t parse_port(const char *port_str);

/* Setup raw socket setting */
int setup_raw_sock(int raw_sockfd);

/* Parse packet data */
void parse_packet(const char *bufline);
void parse_ip(struct iphdr* ip_header);
void parse_udp(struct udphdr* udp_header);
void parse_data(unsigned char *data, int data_len);

/* Listen raw socket */
void *listen_raw_sock(void *arg);


int main(int argc, char* argv[]) 
{
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    if ((raw_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setup_raw_sock(raw_sockfd) == -1) {
        close(raw_sockfd);
        exit(EXIT_FAILURE);
    }
   
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, listen_raw_sock, NULL) != 0) {
        perror("pthread_create failed");
        close(raw_sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_join(recv_thread, NULL);

    close(raw_sockfd);
    return EXIT_SUCCESS;
}

void handle_sigint() {
    running = 0;
}

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int setup_raw_sock(int raw_sockfd) {
    struct sockaddr_in raw_sock_addr;
    memset(&raw_sock_addr, 0, sizeof(raw_sock_addr));
    raw_sock_addr.sin_family = AF_INET;
    raw_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(raw_sockfd, (struct sockaddr *) &raw_sock_addr, sizeof(raw_sock_addr)) == -1) {
        perror("bind failed");
        return -1;
    }

    return 0;
}

void parse_packet(const char *bufline) {
    printf("------------------------------------------------------\n");
    struct iphdr *ip_header = (struct iphdr *)bufline;
    parse_ip(ip_header);

    if (ip_header->protocol == IPPROTO_UDP) {
        struct udphdr *udp_header = (struct udphdr *)(bufline + ip_header->ihl * 4);
        parse_udp(udp_header);
        
        unsigned char *data = (unsigned char *)(udp_header + 1);
        int data_len = ntohs(udp_header->len) - sizeof(struct udphdr);
        parse_data(data, data_len);       
    }

    printf("------------------------------------------------------\n");
}

void parse_ip(struct iphdr* ip_header) {
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_header->saddr, src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_header->daddr, dst_ip, INET_ADDRSTRLEN);

    printf("IP Header:\n");
    printf("    Version: %d\n", ip_header->version);
    printf("    Header Length: %d bytes\n", ip_header->ihl * 4);
    printf("    TTL: %d\n", ip_header->ttl);
    printf("    Protocol: %d\n", ip_header->protocol);
    printf("    Source IP: %s\n", src_ip);
    printf("    Destination IP: %s\n", dst_ip);
}

void parse_udp(struct udphdr* udp_header) {
    printf("UDP Header:\n");
    printf("    Source Port: %d\n", ntohs(udp_header->source));
    printf("    Destination Port: %d\n", ntohs(udp_header->dest));
    printf("    Length: %d\n", ntohs(udp_header->len));
    printf("    Checksum: 0x%X\n", ntohs(udp_header->check));
}

void parse_data(unsigned char *data, int data_len) {
    printf("Data (%d bytes):\n", data_len);
    for (int i = 0; i < data_len && i < 32; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

void *listen_raw_sock(void *arg) {
    char    bufline[MAX_BUF];
    int     buflen = sizeof(bufline) -1;
    int     bytesread = 0;

    while (running) {
        pthread_testcancel();

        bytesread = recvfrom(raw_sockfd, bufline, buflen, 0, NULL, NULL);
        if (bytesread == -1) {
            perror("recvfrom failed");
            break;
        }

        parse_packet(bufline);
    }

    return NULL;
}
