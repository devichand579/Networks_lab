#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/if_packet.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <netdb.h>
#include <stdbool.h>


typedef struct{
    int size;
    char query[32];
}q_string;

typedef struct{
    uint8_t flag;
    uint32_t ip_addr;
}response;

typedef struct{
    uint16_t id;
    uint8_t type;
    uint8_t no_queries;
    q_string queries[8];
}querypack;

typedef struct{
    uint16_t id;
    uint8_t type;
    uint8_t no_responses;
    response responses[8];
}responsepack;

typedef struct q_table{ 
    uint8_t id;
    int no_timeouts;
    char query[1024];
    struct q_table *next;
}q_table;


// Function to calculate the checksum of an IP header
uint16_t calculate_checksum(const uint16_t *data, int length) {
    uint32_t sum = 0;
    const uint16_t *ptr = data;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    // Add any remaining byte
    if (length == 1) {
        sum += *((uint8_t*)ptr);
    }

    // Add carry
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Take one's complement
    return ~sum;
}

bool dropMessage(int probability){
    float random = rand() % 100 / 100.0;
    if(random < probability){
        return true;
    }
    return false;
}

int main()
{
    int prob;
    char interface[32];
    printf("Enter the interface: ");
    scanf("%s", interface);
    printf("Enter the probability of dropping a message: ");
    scanf("%d", &prob);
    int sockid;
    struct sockaddr_ll addr;
    struct ifreq ifr,ifr_2,ifr_3;
    struct ethhdr *et_head;
    struct iphdr *ip_head;
    sockid = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sockid < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
    if(ioctl(sockid, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl failed");
        exit(1);
    }
    if(setsockopt(sockid, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0)
    {
        perror("setsockopt failed");
        exit(1);
    }
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifr.ifr_ifindex;
    if(bind(sockid, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind failed");
        exit(1);
    }
    printf("Socket created and binded successfully\n");
    printf("Interface index is %d\n", ifr.ifr_ifindex);
    memset(&ifr_2, 0, sizeof(ifr_2));
    strncpy(ifr_2.ifr_name, interface, IFNAMSIZ-1);
    if(ioctl(sockid, SIOCGIFADDR, &ifr_2) < 0)
    {
        perror("ioctl failed");
        exit(1);
    }
    printf("IP address of the interface is: %s\n", inet_ntoa(((struct sockaddr_in *)&ifr_2.ifr_addr)->sin_addr));
    memset(&ifr_3, 0, sizeof(ifr_3));
    strncpy(ifr_3.ifr_name, interface, IFNAMSIZ-1);
    if(ioctl(sockid, SIOCGIFHWADDR, &ifr_3) < 0)
    {
        perror("ioctl failed");
        exit(1);
    }
    printf("MAC address of the interface is: %02x:%02x:%02x:%02x:%02x:%02x\n", (unsigned char)ifr_3.ifr_hwaddr.sa_data[0], (unsigned char)ifr_3.ifr_hwaddr.sa_data[1], (unsigned char)ifr_3.ifr_hwaddr.sa_data[2], (unsigned char)ifr_3.ifr_hwaddr.sa_data[3], (unsigned char)ifr_3.ifr_hwaddr.sa_data[4], (unsigned char)ifr_3.ifr_hwaddr.sa_data[5]);
    char *send_buff = (char *)malloc(1024);
    char *recv_buff = (char *)malloc(1024);
    et_head = (struct ethhdr *)send_buff;
    ip_head = (struct iphdr *)(send_buff + sizeof(struct ethhdr));
    et_head->h_proto = htons(ETH_P_IP);
    et_head->h_source[0] = (unsigned char)ifr_3.ifr_hwaddr.sa_data[0];
    et_head->h_source[1] = (unsigned char)ifr_3.ifr_hwaddr.sa_data[1];
    et_head->h_source[2] = (unsigned char)ifr_3.ifr_hwaddr.sa_data[2];
    et_head->h_source[3] = (unsigned char)ifr_3.ifr_hwaddr.sa_data[3];
    et_head->h_source[4] = (unsigned char)ifr_3.ifr_hwaddr.sa_data[4];
    et_head->h_source[5] = (unsigned char)ifr_3.ifr_hwaddr.sa_data[5];
    ip_head->ihl = 5;
    ip_head->version = 4;
    ip_head->tos = 16;
    ip_head->tot_len = 20*sizeof(char) + sizeof(querypack);
    ip_head->id = htonl(1000);
    ip_head->ttl = 64;
    ip_head->protocol = 254;
    ip_head->saddr = inet_addr(inet_ntoa(((struct sockaddr_in *)&ifr_2.ifr_addr)->sin_addr));
    ip_head->check = calculate_checksum((uint16_t *)ip_head, sizeof(struct iphdr));
    responsepack *resp = (responsepack *)(send_buff + sizeof(struct ethhdr) + sizeof(struct iphdr));
    struct sockaddr_ll dest_addr;
    dest_addr.sll_ifindex = ifr.ifr_ifindex;
    dest_addr.sll_halen = ETH_ALEN;
    struct hostent *host;

    while(1){
        int recv_len = recvfrom(sockid, recv_buff, 1024, 0, NULL, NULL);
        if(recv_len < 0)
        {
            perror("recvfrom failed");
            exit(1);
        }
        if(dropMessage(prob)){
            printf("Dropped a message\n");
            continue;
        }

        struct ethhdr *recv_et_head = (struct ethhdr *)recv_buff;
        struct iphdr *recv_ip_head = (struct iphdr *)(recv_buff + sizeof(struct ethhdr));
        
        if(recv_ip_head->protocol == 254)
        {
            querypack *query = (querypack *)(recv_buff + sizeof(struct ethhdr) + sizeof(struct iphdr));
            if(query->type != 0)
            {
                continue;
            }
            printf("Received a UDP packet\n");
            printf("Source IP: %s\n", inet_ntoa(*(struct in_addr *)&recv_ip_head->saddr));
            printf("Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&recv_ip_head->daddr));
            printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", recv_et_head->h_source[0], recv_et_head->h_source[1], recv_et_head->h_source[2], recv_et_head->h_source[3], recv_et_head->h_source[4], recv_et_head->h_source[5]);
            printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", recv_et_head->h_dest[0], recv_et_head->h_dest[1], recv_et_head->h_dest[2], recv_et_head->h_dest[3], recv_et_head->h_dest[4], recv_et_head->h_dest[5]);
            printf("Query ID: %d\n", query->id);
            printf("Query type: %d\n", query->type);
            printf("Number of queries: %d\n", query->no_queries);
            for(int i = 0; i < query->no_queries; i++)
            {
                printf("Query %d: %s\n", i+1, query->queries[i].query);
            }
            resp->id = query->id;
            resp->type = 1;
            resp->no_responses = query->no_queries;
            for(int i = 0; i < query->no_queries; i++)
            {
                host = gethostbyname(query->queries[i].query);
                if(host == NULL)
                {
                    //perror("gethostbyname failed");
                    resp->responses[i].flag = 0;
                    
                }
                else{
                   struct in_addr **addr_list = (struct in_addr **)host->h_addr_list;
                   resp->responses[i].flag = 1;
                   resp->responses[i].ip_addr = inet_addr(inet_ntoa(*addr_list[0]));

                   printf("Response %d: %s\n", i+1, inet_ntoa(*addr_list[0]));
                }
            }
            ip_head->daddr = recv_ip_head->saddr;
            ip_head->check = calculate_checksum((uint16_t *)ip_head, sizeof(struct iphdr)/2);
            et_head->h_dest[0] = recv_et_head->h_source[0];
            et_head->h_dest[1] = recv_et_head->h_source[1];
            et_head->h_dest[2] = recv_et_head->h_source[2];
            et_head->h_dest[3] = recv_et_head->h_source[3];
            et_head->h_dest[4] = recv_et_head->h_source[4];
            et_head->h_dest[5] = recv_et_head->h_source[5];
            dest_addr.sll_addr[0] = recv_et_head->h_source[0];
            dest_addr.sll_addr[1] = recv_et_head->h_source[1];
            dest_addr.sll_addr[2] = recv_et_head->h_source[2];
            dest_addr.sll_addr[3] = recv_et_head->h_source[3];
            dest_addr.sll_addr[4] = recv_et_head->h_source[4];
            dest_addr.sll_addr[5] = recv_et_head->h_source[5];
               
            if(sendto(sockid, send_buff, 1024, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr))<0)
            {
                perror("sendto failed");
                close(sockid);
                exit(1);
            }
        }
    }
    close(sockid);
    return 0;
}

