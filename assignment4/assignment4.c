#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_CLIENTS 3
#define BUFFER_SIZE 256


struct user_info {
    char name[20];
    char ip[20];
    int port;

};

struct user_info user_list[MAX_CLIENTS];


struct user_info user_list[MAX_CLIENTS] = {
    {"Devichand", "127.0.0.1", 50000},
    {"Aman", "127.0.0.1", 50001},
    {"Vinija", "127.0.0.1", 50002}
};


void error(const char *msg) {
    perror(msg);
    exit(1);
}

int connect_to_friend(const char *friend_name, int *client_socket) {
    struct sockaddr_in friend_addr;
    int sock = -1;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(user_list[i].name, friend_name) == 0) {
            // Create a socket for the friend if not already connected
            if (client_socket[i] == 0) {
                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    perror("Socket creation error");
                    return -1;
                }

                memset(&friend_addr, '0', sizeof(friend_addr));
                friend_addr.sin_family = AF_INET;
                friend_addr.sin_port = htons(user_list[i].port);

                if (inet_pton(AF_INET, user_list[i].ip, &friend_addr.sin_addr) <= 0) {
                    printf("\nInvalid address/ Address not supported \n");
                    return -1;
                }

                if (connect(sock, (struct sockaddr *)&friend_addr, sizeof(friend_addr)) < 0) {
                    printf("\nConnection Failed for user %s\n", friend_name);
                    return -1;
                }

                client_socket[i] = sock;
                printf("Connected to %s\n", friend_name);
            } else {
                sock = client_socket[i];
            }
            break;
        }
    }

    return sock;
}

int main(int argc, char *argv[]) {
    int client_socket[MAX_CLIENTS];
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    int activity, i, valread, sd;
    int max_sd;
    fd_set readfds;

    if (argc < 2) {
        fprintf(stderr, "Port number not provided. Program terminated\n");
        exit(1);
    }

    int PORT = atoi(argv[1]);
    address.sin_family = AF_INET;
    inet_aton("127.0.0.1", &address.sin_addr);
    address.sin_port = htons(PORT);

    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Create a master socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        error("socket failed");
    }

    // Set master socket to allow multiple connections
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        error("setsockopt");
    }
    
    // Bind the socket to localhost port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        error("bind failed");
    }
    
    // Specify maximum of 2 pending connections for the master socket
    if (listen(server_fd, 2) < 0) {
        error("listen");
    }

    // Accept incoming connection
    puts("Waiting for connections ...");

    while(1) {
        // Clear the socket set
        FD_ZERO(&readfds);
        
        // Add master socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        FD_SET(0, &readfds); // 0 is the file descriptor for stdin
        
        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            // Socket descriptor
            sd = client_socket[i];
            
            // If valid socket descriptor then add to read list
            if(sd > 0) {
                FD_SET(sd, &readfds);
            }
            
            // Highest file descriptor number, need it for the select function
            if(sd > max_sd) {
                max_sd = sd;
            }
        }
        
        // Wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
    
        if ((activity < 0) && (errno!=EINTR)) {
            printf("select error");
        }
        
        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                error("accept");
            }
      
            
            // Add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                // If position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }
        
    // If there's activity on stdin, process the input
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
    // Read the message from stdin
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Remove newline character from fgets

    // Parse the friend's name and message
    char *friend_name = strtok(buffer, "/");
    char *message = strtok(NULL, "");

    if (friend_name && message) {
        // Find the friend's socket descriptor or establish a new connection
        int friend_socket = -1;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (strcmp(user_list[i].name, friend_name) == 0) {
                if (client_socket[i] != 0) {
                    // Friend is already connected
                    friend_socket = client_socket[i];
                } else {
                    // No connection exists, attempt to connect
                    friend_socket = connect_to_friend(friend_name, client_socket);
                    if (friend_socket != -1) {
                        client_socket[i] = friend_socket; // Store the new socket descriptor
                    }
                }
                break;
            }
        }

        if (friend_socket == -1) {
            printf("Could not find or connect to friend '%s'.\n", friend_name);
        } else {
            // Send the message to the friend's socket
            send(friend_socket, message, strlen(message), 0);
        }
    } else {
        printf("Invalid input format or missing friend name/message.\n");
    }
    }

    
        // Else it's some IO operation on other socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            // Check if the socket descriptor is part of the set
            if (FD_ISSET(sd, &readfds) && sd != 0) {
            // Check if it was for closing, and also read the incoming message
            valread = read(sd, buffer, BUFFER_SIZE - 1);
            if (valread == 0) {
                // Somebody disconnected, get his details and print
                getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                 // Close the socket and mark as 0 in list for reuse
                 close(sd);
                 client_socket[i] = 0;
            } else {
                 buffer[valread] = '\0'; // Null-terminate the incoming message

            // Find out who sent the message
            char *client_name = "";
            printf("Received message from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (ntohs(address.sin_port) == user_list[j].port && 
                    strcmp(inet_ntoa(address.sin_addr), user_list[j].ip) == 0) {
                    printf("Client name: %s\n", user_list[j].name);
                    strcpy(client_name, user_list[j].name);
                    break;
                }
            }
            
            // Display the message on the console with the friend's name
            printf("Message from %s: %s\n", client_name, buffer);
            }
        } 
        }

    }

    return 0;
}

