#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLEN 100

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buf[MAXLEN];
    char filename[MAXLEN];
    int key = 0;
    char key_str[100];
    ssize_t n;

    while (1) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
        perror("Unable to create socket\n");
        exit(0);
        }

       serv_addr.sin_family = AF_INET;
       inet_aton("127.0.0.1", &serv_addr.sin_addr);
       serv_addr.sin_port = htons(8181);

        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Unable to connect to server\n");
        exit(0);
        }

        printf("\nMenu:\n");
        printf("Enter file name to send or 'q' to quit: ");
        scanf("%s", filename);

        if (strcmp(filename, "q") == 0) {
            close(sockfd);
            exit(0);
        }

        int file = open(filename, O_RDONLY);
        if (file < 0) {
            perror("Error opening file");
            continue;
        } else {
            printf("File opened successfully\n");
            printf("Enter the Key: ");
            scanf("%d", &key);
            sprintf(key_str, "%d", key);
            close(file);
        }

        // Send key to the server
        send(sockfd, key_str, sizeof(key_str), 0);
        printf("Key sent to server\n");

        // Send file content to the server
        file = open(filename, O_RDONLY);
        while ((n = read(file, buf, MAXLEN)) > 0) {
            send(sockfd, buf, n, 0);
        }
        send(sockfd, "#", 1, 0);
        close(file);
        printf("File content sent to server\n");

        char enc_filename[MAXLEN];
        sprintf(enc_filename, "%s.enc", filename);

        int enc_file = open(enc_filename, O_WRONLY | O_CREAT, 0666);
        if (enc_file < 0) {
            perror("Error opening file for writing encrypted data");
            exit(0);
        }
        while ((n = recv(sockfd, buf, 1, 0)) > 0 && buf[0] != '#') {
            write(enc_file, buf, 1);
        }

        if (n < 0) {
            perror("Error in recv");
        }

        close(enc_file);
        printf("Encoding complete. Original file: %s, Encrypted file: %s\n", filename, enc_filename);
        close(sockfd);

    }
    return 0;
}
