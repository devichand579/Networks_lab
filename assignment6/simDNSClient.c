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
    char query[8][32];
    int no_queries;
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

bool isValidDomainName(char *domainName){
    // Check if domain name is valid
    int len = strlen(domainName);
    if(len<3 || len>31){
        printf("Invalid domain name\n");
        return false;
    }
    for(int i=0; i<len; i++){
        if((domainName[i] >= 'a' && domainName[i] <= 'z') || (domainName[i] >= 'A' && domainName[i] <= 'Z') || (domainName[i] >= '0' && domainName[i] <= '9') || domainName[i] == '.' || domainName[i] == '-'){
            continue;
        }
        else{
            printf("Invalid domain name\n");
            return false;
        }
    }
    if(domainName[0]=='-' || domainName[len-1]=='-'){
        printf("Invalid domain name\n");
        return false;
    }
    for(int i=0; i<len-1; i++){
        if(domainName[i]=='-' && domainName[i+1]=='.'){
            printf("Invalid domain name\n");
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    char ipadd[32];
    char macadd[32];
    char interface[32];
    sprintf(ipadd,"%s", argv[1]);
    sprintf(macadd,"%s", argv[2]);
    sprintf(interface,"%s", argv[3]);
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
    et_head->h_dest[0] = (unsigned char)strtol(macadd, NULL, 16);
    et_head->h_dest[1] = (unsigned char)strtol(macadd + 3, NULL, 16);
    et_head->h_dest[2] = (unsigned char)strtol(macadd + 6, NULL, 16);
    et_head->h_dest[3] = (unsigned char)strtol(macadd + 9, NULL, 16);
    et_head->h_dest[4] = (unsigned char)strtol(macadd + 12, NULL, 16);
    et_head->h_dest[5] = (unsigned char)strtol(macadd + 15, NULL, 16);
    ip_head->ihl = 5;
    ip_head->version = 4;
    ip_head->tos = 16;
    ip_head->tot_len = 20*sizeof(char) + sizeof(querypack);
    ip_head->id = htonl(1000);
    ip_head->ttl = 64;
    ip_head->protocol = 254;
    ip_head->saddr = inet_addr(inet_ntoa(((struct sockaddr_in *)&ifr_2.ifr_addr)->sin_addr));
    ip_head->daddr = inet_addr(ipadd);
    ip_head->check = calculate_checksum((uint16_t *)ip_head, sizeof(struct iphdr));
    querypack *qp = (querypack *)(send_buff + sizeof(struct ethhdr) + sizeof(struct iphdr));
    struct sockaddr_ll dest_addr;
    dest_addr.sll_ifindex = ifr.ifr_ifindex;
    dest_addr.sll_halen = ETH_ALEN;
    dest_addr.sll_addr[0] =  et_head->h_dest[0];
    dest_addr.sll_addr[1] =  et_head->h_dest[1];
    dest_addr.sll_addr[2] =  et_head->h_dest[2];
    dest_addr.sll_addr[3] =  et_head->h_dest[3];
    dest_addr.sll_addr[4] =  et_head->h_dest[4];
    dest_addr.sll_addr[5] =  et_head->h_dest[5];

    int q_id=0;
    q_table *head = NULL;
    q_table *temp = NULL;
    q_table *prev = NULL;
    char prompt[1024];


    while(1){
        temp = head;
        prev = head;
        while(temp!=NULL){
            if(temp->no_timeouts>=3){
                printf("Query %d timed out\n", temp->id);
                if(temp==head){
                    head = head->next;
                    free(temp);
                    temp = head;
                    prev = head;
                }
                else{
                    prev->next = temp->next;
                    free(temp);
                    temp = prev->next;
                }
            }
            else{
                // retransmit the query
                qp->id = temp->id;
                qp->type = 0;
                qp->no_queries = temp->no_queries;
                for(int i=0; i<temp->no_queries; i++){
                    qp->queries[i].size = strlen(temp->query[i]);
                    strcpy(qp->queries[i].query, temp->query[i]);
                }
                if(sendto(sockid, send_buff, 20*sizeof(char) + sizeof(querypack), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
                {
                    perror("sendto failed");
                    exit(1);
                }
                prev = temp;
                temp = temp->next;

            }
        }
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockid, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int ret = select(sockid+1, &readfds, NULL, NULL, &timeout);
        if(ret<0){
            perror("select failed");
            exit(1);
        }
        else if(ret == 0){
            temp = head;
            while(temp!=NULL){
                temp->no_timeouts++;
                temp = temp->next;
            }
        }
        else {
            if(FD_ISSET(STDIN_FILENO, &readfds)){
                scanf("%[^\n]%*c", prompt);
                if(strcmp(prompt, "EXIT")==0){
                   break;
                }
                temp = (q_table *)malloc(sizeof(q_table));
                // Break the prompt into individual tokens
                char *token;
                char queries[8][32];
                int num = 0;
                int n;
                //printf("%s\n", prompt);
                token = strtok(prompt, " ");
                while (token != NULL) {
                    if(num>=2){
                        sprintf(temp->query[num-2], "%s", token);
                    }
                    if(num==1){
                      n = atoi(token);
                      if(n>8){
                      printf("Invalid number of queries\n");
                      continue;
                      }
                   }
                    token = strtok(NULL, " ");
                   num++;
                }    
                // Check if the remaining tokens are valid domain names
                int flag = 0;
                for (int i = 1; i < n; i++) {
                    if (!isValidDomainName(temp->query[i])) {
                         flag=1; 
                         printf("Invalid domain name\n");
                         break;
                    }
                }
                if(flag==1){
                    continue;
                }
            
                
                temp->id = q_id;
                temp->no_timeouts = 0;
                temp->no_queries = n;
                temp->next = NULL;
                if(head==NULL){
                head = temp;
                }
                else{
                   prev->next = temp;
                }
                prev = temp;
                q_id++;
            }
            else if(FD_ISSET(sockid, &readfds)){
                int recv_len = recvfrom(sockid, recv_buff, 1024, 0, NULL, NULL);
                if(recv_len < 0)
                {
                    perror("recvfrom failed");
                    exit(1);
                }

                // check the ethernet header
                struct ethhdr *eth = (struct ethhdr *)recv_buff;
                if(ntohs(eth->h_proto) != ETH_P_IP){
                    continue;
                }
                // check the ip header
                struct iphdr *ip = (struct iphdr *)(recv_buff + sizeof(struct ethhdr));
                if(ip->protocol != 254){
                    continue;
                }
                if(ip->saddr != inet_addr(ipadd)){
                    continue;
                }
                responsepack *rp = (responsepack *)(recv_buff + sizeof(struct ethhdr) + sizeof(struct iphdr));
                if(rp->type == 0){
                   continue;
                }
                temp = head;
                prev = head;
                char *token;
                while(temp!=NULL){
                    if(rp->id == temp->id){
                        printf("Query %d\n", temp->id);
                        int num_queries = temp->no_queries;
                        printf("Total Query strings: %d\n", num_queries);
                        for(int i=0; i<num_queries; i++){
                            printf("%s", temp->query[i]);
                            if(rp->responses[i].flag==0){
                                printf(" - No IP Address\n");
                            }
                            else{
                                rp->responses[i].ip_addr = ntohl(rp->responses[i].ip_addr);
                                printf(" - %d.%d.%d.%d\n", (rp->responses[i].ip_addr>>24)&0xFF, (rp->responses[i].ip_addr>>16)&0xFF, (rp->responses[i].ip_addr>>8)&0xFF, rp->responses[i].ip_addr&0xFF);
                            }
                        }
                        printf("\n");
                        if(temp==head){
                            head = head->next;
                            free(temp);
                            temp = head;
                            prev = head;
                        }
                        else{
                            prev->next = temp->next;
                            free(temp);
                            temp = prev->next;
                        }
                    }
                    else{
                        prev = temp;
                        temp = temp->next;
                    }
                }
            }
        
        }

    }
    free(send_buff);
    free(recv_buff);
    close(sockid);
    return 0;
}

