// A Simple UDP Server that sends a HELLO message
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
    int sockid; 
    struct sockaddr_in serveraddr, clientaddr; 
    int n; 
    socklen_t len;
    char filename[MAXLEN]; 
    char mssg[MAXLEN] = "NOTFOUND";
    char buffer[MAXLEN];

    // Create socket file descriptor 
    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockid < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    memset(&serveraddr, 0, sizeof(serveraddr));
    memset(&clientaddr, 0, sizeof(clientaddr));

    serveraddr.sin_family    = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(8181);

    // Bind the socket with the server address
    if ( bind(sockid, (const struct sockaddr *)&serveraddr,  
            sizeof(serveraddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }

    printf("\nServer Running....\n");
    len = sizeof(clientaddr);
    n = recvfrom(sockid, (char *)filename, MAXLEN, 0, 
            ( struct sockaddr *) &clientaddr, &len);
    filename[n] = '\0';
    printf("Recieved File Name : %s\n", filename);
    DIR *dir;
    struct dirent *entry;

    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir failed");
        exit(EXIT_FAILURE);
    }

    int fileFound = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, filename) == 0) {
            fileFound = 1;
            break;
        }
    }
    if (fileFound==0) {
        printf("File '%s' not found in the local directory.\n", filename);
        strcat(mssg, filename);
        sendto(sockid, (const char *)mssg, strlen(mssg), 0, 
            (const struct sockaddr *) &clientaddr, sizeof(clientaddr)); 
        closedir(dir);
        exit(EXIT_FAILURE);   
    } else {
        printf("File '%s' found in the local directory.\n", filename);
    }
    FILE *file = fopen(filename, "r");
        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        // Read and print the contents of the file
        char line[MAXLEN];
        while (fgets(line, sizeof(line), file) != NULL) {
        // Tokenize the line into words
        char *token = strtok(line, " ");
        
        // Send each word separately
        while (token != NULL) {
            // Find the index of the newline character, if present
            size_t newlineIndex = strcspn(token, "\n");

            // Send only the part of the token before the newline
            sendto(sockid, (const char *)token, newlineIndex, 0,
                (const struct sockaddr *) &clientaddr, sizeof(clientaddr));
            printf("Token sent: %.*s\n", (int)newlineIndex, token);

            // Receive acknowledgment or response from the client
            n = recvfrom(sockid, (char *)buffer, MAXLEN, 0,
                (struct sockaddr *) &clientaddr, &len);
            printf("Message Received: %s\n", buffer);

            // Get the next word
            token = strtok(NULL, " ");
        }
    }
        // Close the file
    fclose(file);
    closedir(dir);
    close(sockid);
    return 0;

}