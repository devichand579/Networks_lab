#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msocket.h"

#define BUFSIZE 1023
#define SOCK_MTP 1001



int main() {
    int sockfd;
    char *filename = "Sample200.txt";
    char buffer[BUFSIZE];

    // Create MTP socket
    sockfd = m_socket( AF_INET, SOCK_MTP, 0 ); // Specify IP_1, Port_1, IP_2, Port_2

    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    struct sockaddr_in addr2;
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(8081);
    addr2.sin_addr.s_addr = INADDR_ANY;

    // Bind MTP socket
    int x = m_bind(sockfd, (struct sockaddr*)&addr, sizeof(addr), (struct sockaddr*)&addr2,sizeof(addr2)); // Specify IP_1, Port_1, IP_2, Port_2
    if(x < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }
    // Open file to send
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int i=1,r;
    while ( (r=fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
        int x = m_sendto(sockfd,  buffer, BUFSIZE, 0, (struct sockaddr*)&addr2,sizeof(addr2));
        while(x == -1){
            sleep(5);
            x = m_sendto(sockfd,  buffer, BUFSIZE, 0, (struct sockaddr*)&addr2,sizeof(addr2));
        }
        printf("Message sent %d\n",i);
        i++;
    }

    fclose(file);
    m_close(sockfd);

    return 0;
}
