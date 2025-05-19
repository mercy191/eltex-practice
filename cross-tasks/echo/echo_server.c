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
    struct in_addr client_ip;
    uint16_t client_port;
    int counter;
} ClientInfo;

volatile int        running = 1;
int                 server_raw_sockfd = -1;
uint32_t            server_ip = 0;
uint16_t            server_port = 0;
int                 clients_count = 0;
ClientInfo          clients_info[MAX_CLIENT];


/* SIGINT handle */
void handle_sigint();

/* Write in STDOUT */
void write_stdout(const char *str);

/* Parse IP and port */
int parse_ip_port(char* cserver_ip, char* cserver_port);

/* Setup socket settings*/
int setup_socket();

/* Add information about client in CLIENTS_INFO */
int add_client(ClientInfo* client_info);

/* Reset information about client session in CLIENTS_INFO */
void reset_client(ClientInfo* client_info);

/* Clear information about client session in CLIENTS_INFO */
void clear_client(ClientInfo* client_info) ;

/* Find client in CLIENTS_INFO */
int find_client(ClientInfo* client_info);

/* Deals with the processing and sending of messages */
void* processing_thread(void *arg); 

/* Receive client packet and parse it in CLIENT_INFO and CLIENT_MSG */
int receive_packet(ClientInfo* client_info, char* client_msg); 

/* Configurate server packet from CLIENT_INFO and SERVER_MSG and send it */
int send_packet(ClientInfo* client_info, char* server_msg); 

/* Parse IP packet, fill CLIENT_INFO and CLIENT_MSG */
int get_client_msg_from_packet(ClientInfo* client_info, char* client_msg, char* packet, uint16_t packet_len);

/* Configurate new IP packet from CLIENT_INFO and SERVER_MSG */
int set_server_msg_in_packet(ClientInfo* client_info, char* server_msg, char* packet, uint16_t packet_len);

/* Configurate SERVER_MSG from CLIENT_INFO and CLIENT_MSG */
void configurate_server_msg(char* client_msg, char* server_msg);


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

    if (parse_ip_port(argv[1], argv[2]) == -1) {
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

    if (setup_socket() == -1) {
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

int parse_ip_port(char* cserver_ip, char* cserver_port) {
    if (inet_pton(AF_INET, cserver_ip, &server_ip) == -1) {
        perror("inet_pton failed");
        return -1;
    } 
    server_port = htons(strtol(cserver_port, NULL, 10));

    return 0;
}

int setup_socket() {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_addr.s_addr = server_ip;
    server_addr.sin_port = server_port;
    server_addr.sin_family = AF_INET;
    
    if (bind(server_raw_sockfd, (struct sockaddr*) &server_addr, (socklen_t) sizeof(struct sockaddr))) {
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
        clients_info[clients_count].client_ip = client_info->client_ip;
        clients_info[clients_count].client_port = client_info->client_port;
        clients_info[clients_count].counter = 1;
        clients_count++;
    }  
    
    return 0;
}

void reset_client(ClientInfo* client_info) {

}

void clear_client(ClientInfo* client_info) {

}

int find_client(ClientInfo* client_info) {
    for (int i = 0; i < clients_count; i++) {
        if (clients_info[i].client_ip.s_addr == client_info->client_ip.s_addr && clients_info[i].client_port == client_info->client_port) {
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
        if (receive_packet(&client_info, client_msg) == -1) {
            continue;
        }

        if (strncmp(client_msg, EXIT_MSG, sizeof(EXIT_MSG)) == 0) {
            clear_client(&client_info);
            continue;
        }

        if (find_client(&client_info) == -1) {
            if (add_client(&client_info) == -1) {
                continue;
            }
        }
        else {
            reset_client(&client_info);
        }



        

    }

    return NULL;
}


int receive_packet(ClientInfo* client_info, char* client_msg) {
    char*   packet;
    int     packet_len;

    /* Read ip packet length */
    uint16_t ip_length[2];
    int recv_bytes = recv(server_raw_sockfd, (char*) ip_length, 4, MSG_PEEK);
    packet_len = ntohs(ip_length[1]);

    /* Allocate memory for ip packet */
    packet = malloc(packet_len);

    /* Read full ip packet */
    if ((recv_bytes = recv(server_raw_sockfd, packet, packet_len, 0)) == -1) {
        perror("recv failed");
        free(packet);
        return -1;
    }

    /* Copy msg from ip packet */ 
    if (get_client_msg_from_packet(client_info, client_msg, packet, packet_len) == -1) {
        free(packet);
        return -1;
    }
    else {
        char log_msg[LOG_SIZE];
        sprintf(log_msg, "Client %s:%d connected\n", inet_ntoa(client_info->client_ip), client_info->client_port);
        write_stdout(log_msg);
    }
     
    free(packet);
    return 0;
}

int send_packet(ClientInfo* client_info, char* server_msg) {

}

int get_client_msg_from_packet(ClientInfo* client_info, char* client_msg, char* packet, uint16_t packet_len) {
    struct iphdr* ip_header = (struct iphdr*) packet;
    struct udphdr* udp_header = (struct udphdr*) (ip_header + sizeof(struct iphdr));
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_len = packet_len - sizeof(struct udphdr) - sizeof(struct iphdr);

    printf("IP header size: %ld\n", sizeof(struct iphdr));
    printf("IP total length: %u\n", ntohs(ip_header->tot_len));
    printf("IP source address: %u\n", ntohl(ip_header->saddr));
    printf("IP dest address: %u\n", ntohl(ip_header->daddr));

    printf("UDP header size: %ld\n", sizeof(struct udphdr));
    printf("UDP source port: %u\n", ntohs(udp_header->source));
    printf("UDP dest port: %u\n", ntohs(udp_header->dest));
    printf("UDP total length: %u\n", ntohs(udp_header->len));
    printf("UDP payload size: %d\n", payload_len);

    /*if (udp_header->dest != server_port) {
        return -1;
    }*/

    client_info->client_ip.s_addr = ip_header->saddr;
    client_info->client_port = udp_header->source;
    client_info->counter = 1;

    client_msg = malloc(payload_len);
    strncpy(client_msg, payload, payload_len);
    
    return 0;
}

int set_server_msg_in_packet(ClientInfo* client_info, char* server_msg, char* packet, uint16_t packet_len) {
    struct iphdr* ip_header = (struct iphdr*) packet;
    struct udphdr* udp_header = (struct udphdr*) (packet + sizeof(struct iphdr));
    char* payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_len = packet_len - sizeof(struct udphdr) - sizeof(struct iphdr);


    ip_header->ihl = 5;
    ip_header->version = 4;
    ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_len);
    ip_header->ttl = 64;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->saddr = server_ip;
    ip_header->daddr = client_info->client_ip.s_addr;

    udp_header->source = server_port;
    udp_header->dest = client_info->client_port;
    udp_header->len = htons(sizeof(struct udphdr) + payload_len);
}

void configurate_server_msg(char* client_msg, char* server_msg) {

}