#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>

#define MAXLEN 100

int main() {
    int sockid, err;
    struct sockaddr_in servaddr;
    int n;
    socklen_t len;
    char file_name[MAXLEN];
    char mssg[MAXLEN];
    char MSSG[MAXLEN] = "NOTFOUND";
    char buffer[MAXLEN];
    FILE *file=NULL;

    // Creating socket file descriptor
    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8181);
    err = inet_aton("10.145.238.128", &servaddr.sin_addr);
    if (err == 0) {
        printf("Error in ip-conversion\n");
        exit(EXIT_FAILURE);
    }

    printf("Enter the file name to be searched: ");
    scanf("%s", file_name);
    strcat(MSSG, file_name);

    sendto(sockid, (const char *)file_name, strlen(file_name), 0,
           (const struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("File Name sent to the server\n");
    len = sizeof(servaddr);
    n = recvfrom(sockid, (char *)mssg, MAXLEN, 0,
                 (struct sockaddr *)&servaddr, &len);
    mssg[n] = '\0';
    printf("Message Received: %s\n", mssg);
    if (strcmp(mssg, MSSG) == 0) {
        printf("File %s Not Found\n", file_name);
        close(sockid);
        exit(EXIT_FAILURE);
    } else {
        printf("File Found\n");
        if (strcmp(mssg, "HELLO") == 0) {
            // Open the file for writing
            file = fopen("result.txt", "w");
            if (file == NULL) {
                perror("Error creating file");
                close(sockid);
                exit(EXIT_FAILURE);
            }
        }
    }

    char word[MAXLEN];
    int i = 1;

    while (1) {
    sprintf(word, "WORD%d", i);
    sendto(sockid, (const char *)word, strlen(word), 0,
           (const struct sockaddr *)&servaddr, sizeof(servaddr));
    n = recvfrom(sockid, (char *)buffer, MAXLEN, 0,
                 (struct sockaddr *)&servaddr, &len);
    if (n < 0) {
        perror("recvfrom error");
        fclose(file);
        close(sockid);
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';
    if (strcmp(buffer, "END") == 0) {
        break; // Exit the loop when "END" is received
    }
    fprintf(file, "%s\n", buffer);
    i++;
}

    fclose(file);
    close(sockid);
    return 0;
}
