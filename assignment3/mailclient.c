#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>

int is_valid_email_address(const char *address) {
    int at_count = 0;
    const char *at_sign = NULL;

    // The local part of the email address cannot begin with '@' and cannot be empty
    if (address[0] == '@' || address[0] == '\0') {
        return 0;
    }

    for (const char *c = address; *c; ++c) {
        if (*c == '@') {
            at_count++;
            at_sign = c;
        } else if (!isalnum((unsigned char)*c) && *c != '.' && *c != '-' && *c != '_') {
            // Invalid character in email address
            return 0;
        }
    }

    // There must be exactly one '@' character, and it cannot be the last character
    if (at_count != 1 || *(at_sign + 1) == '\0') {
        return 0;
    }

    // Check if there's at least one '.' after '@'
    if (strchr(at_sign, '.') == NULL) {
        return 0;
    }

    return 1;
}

int is_valid_email(const char *email) {
    const char *from_header = "From: ";
    const char *to_header = "To: ";
    const char *subject_header = "Subject: ";
    const char *end_of_headers = "\n"; // Headers and body are separated by a blank line
    const char *end_of_email = ".\n";  // Email ends with a line containing only a period

    // Check for 'From' header
    if (strncmp(email, from_header, strlen(from_header)) != 0) {
        return 0; // 'From' header not found
    }

    // Check for 'To' header
    if (strstr(email, to_header) == NULL) {
        return 0; // 'To' header not found
    }

    // Check for 'Subject' header
    if (strstr(email, subject_header) == NULL) {
        return 0; // 'Subject' header not found
    }

    // Check for separation between headers and body
    if (strstr(email, end_of_headers) == NULL) {
        return 0; // Headers and body separation not found
    }

    // Check for end of email
    if (strstr(email, end_of_email) == NULL) {
        return 0; // End of email indicator not found
    }

    return 1; // The email string passed all checks
}

char* validate_from_address(const char *email_string) {
    // Assuming the format is "From: X@Y\n"
    // First find the start of the email address after "From: "
    char *ret = "#";
    const char *start_of_address = strstr(email_string, "From: ");
    if (start_of_address == NULL) {
        return ret; // The "From: " prefix is not found
    }
    start_of_address += 6; // Skip past "From: "

    // Find the end of the email address at the newline character
    const char *end_of_address = strchr(start_of_address, '\n');
    if (end_of_address == NULL) {
        return ret; // Newline not found, invalid format
    }

    // Create a temporary buffer to hold the email address
    size_t length_of_address = end_of_address - start_of_address;
    char *address_buffer = malloc(length_of_address + 1); // Allocate memory for the substring
    strncpy(address_buffer, start_of_address, length_of_address);
    address_buffer[length_of_address] = '\0'; // Null-terminate the string
    // Use the is_valid_email_address function to validate
    return address_buffer;
    

}

char* validate_to_address(const char *email_string) {
    // Assuming the format is "To: X@Y\n"
    // First find the start of the email address after "To: "
    char *ret = "#";
    const char *start_of_address = strstr(email_string, "To: ");
    if (start_of_address == NULL) {
        return ret; // The "To: " prefix is not found
    }
    start_of_address += 4; // Skip past "To: "

    // Find the end of the email address at the newline character
    const char *end_of_address = strchr(start_of_address, '\n');
    if (end_of_address == NULL) {
        return ret; // Newline not found, invalid format
    }

    // Create a temporary buffer to hold the email address
    size_t length_of_address = end_of_address - start_of_address;
    char *address_buffer = malloc(length_of_address + 1);
    strncpy(address_buffer, start_of_address, length_of_address);
     address_buffer[length_of_address] = '\0'; // Null-terminate the string
    // Use the is_valid_email_address function to validate
    return address_buffer;


}

void send_email_body(int socket_fd, const char *body) {
    const char *line_start = body;
    const char *line_end;
    char line_buffer[1024]; // Adjust size as needed

    while ((line_end = strchr(line_start, '\n')) != NULL) {
        int line_length = line_end - line_start;

        // Check if the current line is just ".\n"
        if (line_length == 1 && *line_start == '.') {
            break;
        }

        memcpy(line_buffer, line_start, line_length);
        line_buffer[line_length] = '\r';
        line_buffer[line_length + 1] = '\n';
        send(socket_fd, line_buffer, line_length + 2, 0);
        line_start = line_end + 1;
    }

    // Send the final ".\r\n" to indicate the end of data
    send(socket_fd, ".\r\n", 3, 0);
}

char* extract_domain(const char *str) {
    char *start = strchr(str, '<');
    char *end = strchr(str, '>');

    if (start != NULL && end != NULL && end > start) {
        start++; // Move past '<'
        size_t length = end - start;
        char *domain = malloc(length + 1); // Allocate memory for the substring

        if (domain) {
            strncpy(domain, start, length);
            domain[length] = '\0'; // Null-terminate the string
            return domain;
        }
    }
    return NULL;
}

void recv_mssg(int socket_fd, char* buffer,char* body, size_t buffer_size, const char *expected_response, int key) {
    int n=recv(socket_fd, buffer, buffer_size, 0);
    if (strncmp(buffer, expected_response, strlen(expected_response)) == 0) {
        buffer[n] = '\0';
    }
    else {
        buffer[n] = '\0';
        printf("Error: <%s>\n", buffer);
        exit(0);
    }
    int j=0;
    while( (n= recv(socket_fd, buffer, buffer_size, 0))>0){
        buffer[n] = '\0';
        int i=0;
        if(key==1){
        printf("%s",buffer);
        }
        while(i<n){
            body[j++]=buffer[i];
            if(i-1>=0){
                if(buffer[i]=='.' && buffer[i-1]=='\n'){
                    body[j+1]='\r';
                    body[j+2]='\n';
                    body[j+3]='\0';
                    return;
                }
            }else{
                if(buffer[i]=='.'){
                    body[j+1]='\r';
                    body[j+2]='\n';
                    body[j+3]='\0';
                    return;
                }
            }
            i++;
        }
    }
}

// Helper function to receive an SMTP response and check if it starts with expected_response
void recv_smtp_response(int socket_fd, char *buffer, size_t buffer_size, const char *expected_response) {
   int n=recv(socket_fd, buffer, buffer_size, 0);
    if (strncmp(buffer, expected_response, strlen(expected_response)) == 0) {
        buffer[n] = '\0';
        return;
    }
    else {
        buffer[n] = '\0';
        printf("Error: <%s>\n", buffer);
        exit(0);
    }

}

// Helper function to send an SMTP command and wait for a specific response
void send_smtp_command(int socket_fd, const char *command, char *buffer, size_t buffer_size, const char *expected_response) {
    send(socket_fd, command, strlen(command), 0);
    recv_smtp_response(socket_fd, buffer, buffer_size, expected_response);
}

void send_pop3_command(int socket_fd, const char *command, char *buffer,char* body, size_t buffer_size, const char *expected_response, int key) {
    send(socket_fd, command, strlen(command), 0);
    recv_mssg(socket_fd, buffer, body, buffer_size, expected_response,key);
}


// Pseudo code for SMTP conversation in C
void smtp_conversation(int socket_fd, const char *body, const char *from2, const char *to2) {
    char buffer[1024];


    // Send MAIL FROM command and wait for 250 response
    char mail_from_command[1024];
    snprintf(mail_from_command, sizeof(mail_from_command), "MAIL FROM: <%s>\r\n", from2);
    send_smtp_command(socket_fd, mail_from_command, buffer, sizeof(buffer), "250");
    printf("%s\n",buffer);

    // Send RCPT TO command and wait for 250 response
    char rcpt_to_command[1024];
    snprintf(rcpt_to_command, sizeof(rcpt_to_command), "RCPT TO: <%s>\r\n", to2);
    send_smtp_command(socket_fd, rcpt_to_command, buffer, sizeof(buffer), "250");
    printf("%s\n",buffer);

    // Send DATA command and wait for 354 response
    char data_command[1024];
    snprintf(data_command, sizeof(data_command), "DATA\r\n");
    send_smtp_command(socket_fd, data_command, buffer, sizeof(buffer), "354");
    printf("%s\n",buffer);

    // Send email body and end with <CRLF>.<CRLF>
    send_email_body(socket_fd, body);

    // Wait for 250 response after sending email body
    recv_smtp_response(socket_fd, buffer, sizeof(buffer), "250");
    printf("%s\n",buffer);

    // Send QUIT command and wait for 221 response
    
    send_smtp_command(socket_fd, "QUIT\r\n", buffer, sizeof(buffer), "221");
    printf("%s\n",buffer);
    return;
}



void retrive_sendermail_sub_date(char *body) {
    int i=6;
    while(body[i+1]!='\n'){
        printf("%c",body[i]);
        i++;
    }
    printf(" ");
    i+=6;
    while(body[i+1]!='\n'){
        i++;
    }
    i+=2;
    i+=9;
    while(body[i+1]!='\n'){
        printf("%c",body[i]);
        i++;
    }
    printf(" ");
    i+=2;
    i+=10;
    while(body[i+1]!='\n'){
        printf("%c",body[i]);
        i++;
    }
    printf("\n");
}



void print_mail(char *buff){
    char *token = strtok(buff, "\n");
    while(token != NULL){
        printf("%s\n",token);
        token = strtok(NULL, "\n");
    }
}

void pop3_conversation(int socket_fd, const char *username, const char *password) {
    char buffer[1024];
    char body[4500];
    char c='r';
    
    // Wait for the "+OK server ready <domain>" response
    recv_smtp_response(socket_fd, buffer, sizeof(buffer), "+OK");
    printf("%s\n",buffer);

    // Send USER command and wait for +OK response
    char user_command[1024];
    snprintf(user_command, sizeof(user_command), "USER %s\r\n", username);
    send_smtp_command(socket_fd, user_command, buffer, sizeof(buffer), "+OK");
    printf("%s\n",buffer);

    // Send PASS command and wait for +OK response
    char pass_command[1024];
    snprintf(pass_command, sizeof(pass_command), "PASS %s\r\n", password);
    send_smtp_command(socket_fd, pass_command, buffer, sizeof(buffer), "+OK");
    printf("%s\n",buffer);

    // Send STAT coomand and wait for +OK response and get the number of messages in the mailbox
    while(c!='d'){
       send_smtp_command(socket_fd, "STAT\r\n", buffer, sizeof(buffer), "+OK");
       printf("%s\n",buffer);
       char *token = strtok(buffer, " ");
       token = strtok(NULL, " ");
       int num_messages = atoi(token);
       if(num_messages==0){
           printf("No mails\n");
           return;
       }
       for(int i=1;i<=num_messages;i++){
           char retr_command[1024];
           snprintf(retr_command, sizeof(retr_command), "RETR %d\r\n", i);
           send_pop3_command(socket_fd, retr_command, buffer, body, sizeof(buffer), "+OK",0);
           printf("%d.",i);
           retrive_sendermail_sub_date(body);
       }
       printf("Enter mail no. to see: ");
       int mail_no;
       scanf("%d",&mail_no);
       if(mail_no==-1){
              c='d';
              break;
       }
       if(mail_no < 1 || mail_no > num_messages){
           printf("Enter the number again\n");
           continue;
       }
       char retr_command[1024];
       snprintf(retr_command, sizeof(retr_command), "RETR %d\r\n", mail_no);
       send_pop3_command(socket_fd, retr_command, buffer,body, sizeof(buffer), "+OK",1);
       fflush(stdin);
       fflush(stdout);
       printf("Enter 'd' to delete the mail or any other character to return to list of emails: ");
       int ch =getchar();
       if(ch=='d'){
           char dele_command[1024];
           snprintf(dele_command, sizeof(dele_command), "DELE %d\r\n", mail_no);
           send_smtp_command(socket_fd, dele_command, buffer, sizeof(buffer), "+OK");
           printf("%s\n",buffer);
           c='d'; 

           //Send QUIT command and wait for +OK response
            send_smtp_command(socket_fd, "QUIT\r\n", buffer, sizeof(buffer), "+OK");
            printf("%s\n",buffer);
       }
       else{
           c='r';
       }

    }
    return;
}


// Function to send email
void send_email(int socket_fd) {
    char from[60], to[60], subject[61], body[4300], line[81];
    int valid, valid_from, valid_to;;
    printf("From: (include From field)\n ");
    fflush(stdin);
    fgets(from, 60, stdin);
    char *from2 =  validate_from_address(from);
    printf("%s\n",from2);
    valid_from = is_valid_email_address(from2);
    printf("To: (include To field)\n ");
    fflush(stdin);
    fgets(to, 60, stdin);
    char *to2 = validate_to_address(to);
    valid_to = is_valid_email_address(to2);
    printf("Subject: (include Subject field)\n ");
    fflush(stdin);
    fgets(subject, 61, stdin);

    // Start writing the email body
    printf("Enter the email body and end with a fullstop on a new line.\n");
    strcpy(body, ""); // Initialize the body string
    strcat(body,from);
    strcat(body,to);
    strcat(body,subject);
    while (1) {
        fgets(line, 81, stdin);
        // Check if the line is just a fullstop indicating end of message
        if (strcmp(line, ".\n") == 0) {
            strcat(body, line); // Append the final line
            break;
        }
        // Check the 80 characters limit per line
        if (strlen(line) > 80) {
            printf("Line exceeds 80 characters limit.\n");
            continue;
        }
        strcat(body, line); // Append the line to body
    }
    valid = is_valid_email(body);

    // Wait for the 220 "Service Ready" response
    char buffer[1024];
    recv_smtp_response(socket_fd, buffer, sizeof(buffer), "220");
    char *domain = extract_domain(buffer);
    printf("%s\n",buffer);

    // Send HELO command and wait for 250 response
    char helo_command[1024];
    snprintf(helo_command, sizeof(helo_command), "HELO %s\r\n", domain);
    send_smtp_command(socket_fd, helo_command, buffer, sizeof(buffer), "250");
    printf("%s\n",buffer);

    if(valid == 0 || valid_from == 0 || valid_to == 0){
        printf("Incorrect format\n");
        return;
    }
    smtp_conversation(socket_fd, body, from2, to2);
    return;
}



// Main program that connects to the SMTP server
int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr, serv_addr_pop3;
    char ip_adr[16];
    char usrname[100],passwd[100];
    int option;
    if (argc != 4) {
        printf("Proivde SERVER_IP, smtp_port, pop3_port as command line arguments\n");
        exit(0);
    }
    strcpy(ip_adr, argv[1]);
    int smtp_port = atoi(argv[2]);
    int pop3_port = atoi(argv[3]);

    serv_addr.sin_family = AF_INET;
    inet_aton(ip_adr, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(smtp_port);

    serv_addr_pop3.sin_family = AF_INET;
    inet_aton(ip_adr, &serv_addr_pop3.sin_addr);
    serv_addr_pop3.sin_port = htons(pop3_port);

    printf("Enter username: ");
    scanf("%s",usrname);
    printf("Enter password: ");
    scanf("%s",passwd);

    while(1){
        printf("1. Manage Mail: Show the stored mails of the logged in user only\n");
        printf("2. Send Mail: Allows the user to send a mail\n");
        printf("3. Quit: Quits the program\n");
        printf("Enter option: ");
        scanf("%d",&option);
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Unable to create socket\n");
                exit(0);
            }
        if(option == 1){
            
            if ((connect(sockfd, (struct sockaddr *) &serv_addr_pop3,
                                sizeof(serv_addr_pop3))) < 0) {
                perror("Unable to connect to server\n");
                exit(0);
            }
            printf("< Client connects to POP3 port>\n");
            pop3_conversation(sockfd, usrname, passwd);
            printf("<Client hangs up>\n");
            close(sockfd);
        }
        else if(option == 2){
    

            if ((connect(sockfd, (struct sockaddr *) &serv_addr,
                                sizeof(serv_addr))) < 0) {
                perror("Unable to connect to server\n");
                exit(0);
            }
            printf("< Client connects to SMTP port>\n");
            send_email(sockfd);
            printf("<Client hangs up>\n");
            close(sockfd);
        }
        else if(option == 3){
            break;
        }
        else{
            printf("Invalid option\n");
        }
    }
    return 0;

}

