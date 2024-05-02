
#include "msocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFSIZE 100



int main() {
    int sockfd;
    char *filename = "New.txt";
    char buffer[BUFSIZE];

    // Create MTP socket
    sockfd = m_socket( AF_INET, SOCK_MTP, 0 ); // Specify IP_1, Port_1, IP_2, Port_2

    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = INADDR_ANY;

    struct sockaddr_in addr2;
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(8080);
    addr2.sin_addr.s_addr = INADDR_ANY;

    // Bind MTP socket
    m_bind(sockfd, (struct sockaddr*)&addr, sizeof(addr), (struct sockaddr*)&addr2,sizeof(addr2)); // Specify IP_1, Port_1, IP_2, Port_2

    // Open file to send
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int i=1,recv_len;
    int len = sizeof(addr2);
    while (1) {
        recv_len = m_recvfrom(sockfd, buffer, sizeof(buffer),0, (struct sockaddr*)&addr2, &len);
        if(recv_len==-1){
            sleep(5);
            continue;
        }
        printf("Recieved message %d\n",i);
        fwrite(buffer, sizeof(char), recv_len, file);
        if(recv_len<1023){
            break;
        }
        i++;
    }
    
    fclose(file);
    m_close(sockfd);

    return 0;
}
