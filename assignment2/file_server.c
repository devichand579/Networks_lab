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

void caesarCipher(char *text, int key);

int main() {
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;
    char buf[MAXLEN];
    int key=0;
    char key_str[100];
    ssize_t n;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Cannot create socket\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8181);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Unable to bind local address\n");
        exit(0);
    }
    else {
        printf("Server started\n");
    }

    listen(sockfd, 5);
    
    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            printf("Accept error\n");
            exit(0);
        }

        if (fork() == 0) {
            close(sockfd);

            recv(newsockfd, key_str, MAXLEN, 0);
            key = atoi(key_str); 
            printf("Key received from client: %d\n", key);

            char filename[100];
            sprintf(filename, "%s:%d.txt", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            
            int file = open(filename, O_WRONLY | O_CREAT, 0666);

            while ((n = recv(newsockfd, buf, 1, 0)) > 0 && buf[0] != '#') {
                write(file, buf, 1);
            }

            close(file);

            printf("File received from client\n");

            file = open(filename, O_RDONLY);
            char enc_filename[MAXLEN];
            sprintf(enc_filename, "%s.enc", filename);
            int enc_file = open(enc_filename, O_WRONLY | O_CREAT, 0666);
            while ((n = read(file, buf, MAXLEN)) > 0) {
                caesarCipher(buf, key);
                write(enc_file, buf, n);
            }
            close(file);
            close(enc_file);

            enc_file = open(enc_filename, O_RDONLY);
            while ((n = read(enc_file, buf, MAXLEN)) > 0) {
                send(newsockfd, buf, n, 0);
            }
            send(newsockfd, "#", 1, 0);
            close(enc_file);

            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }

    return 0;
}

void caesarCipher(char *text, int key) {
    for(int i = 0; text[i] != '\0'; ++i){
        char ch = text[i];

        // Encrypt for lowercase letter
        if(ch >= 'a' && ch <= 'z'){
            ch = ch + key;

            if(ch > 'z'){
                ch = ch - 'z' + 'a' - 1;
            }

            text[i] = ch;
        }
        // Encrypt for uppercase letter
        else if(ch >= 'A' && ch <= 'Z'){
            ch = ch + key;

            if(ch > 'Z'){
                ch = ch - 'Z' + 'A' - 1;
            }

            text[i] = ch;
        }
    }
}
