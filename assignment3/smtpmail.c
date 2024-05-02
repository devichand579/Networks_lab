#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h> 


void handleClient(int clientSocket) {
    char buf[101];

    //send only mail address
    int rec_bytes=recv(clientSocket,buf,100,0);
    buf[rec_bytes]='\0';
    printf("%s\n",buf);
    
    char str[5];
    for(int i=0;i<4;i++){
        str[i]=buf[i];
    }
    str[4]='\0';
    int i=0,j=-1;
    char from[100];
    if(strcmp(str,"MAIL\0")==0){
        while(buf[i]!='>'){
            if(j>=0){
                from[j]=buf[i];
                j++;
            }
            if(buf[i]=='<'){
                j++;
            }
            i++;
        }
        char senderok[200];
        // printf("%s\n",from);
        snprintf(senderok,sizeof(senderok),"250 <%s>...Sender ok\r\n",from);
        send(clientSocket,senderok,strlen(senderok),0);

    }



	rec_bytes=recv(clientSocket,buf,100,0);
    buf[rec_bytes]='\0'; 
    printf("%s\n",buf);
     

    for(int i=0;i<4;i++){
        str[i]=buf[i];
    }
    str[4]='\0';

    if(strcmp(str,"RCPT\0")!=0) {
        //do error handling
    }  

    i=10;j=0;
    char to[100];
    while(1){
        if(buf[i]=='@'){
            break;
        }
        to[j]=buf[i];
        j++;
        i++;
    }
    // to[j]='\0';
    
    FILE *uf=fopen("user.txt","r");
    if(uf==NULL){
    	perror("Error in opening the file user.txt");
    	exit(EXIT_FAILURE);
    }
    char line[500];
    int o=0;
    while(fgets(line, sizeof(line), uf)!=NULL){
    	char to_addr[200];
        if (sscanf(line, "%s", to_addr) == 1) {
            // Compare the extracted to_addr with the target to_addr
            //check whether to_addr is null terminated
            if (strcmp(to_addr, to) == 0) {
                o++;  // Username found
                break;
            }
        }
    }
    fclose(uf);
    char fullPath[500];
    char filePath[1000];
    if(o){
	    if (getcwd(fullPath, sizeof(fullPath)) != NULL) {
	        
	        snprintf(filePath, sizeof(filePath), "%s/%s/%s", fullPath, to, "mymailbox");
	    } else {
	        perror("Error getting current working subdirectory");
	    }
        char to_ok[200];
        snprintf(to_ok,sizeof(to_ok),"250 root... Recipient ok\r\n");
        send(clientSocket,to_ok,strlen(to_ok),0);
        printf("%s\n",to_ok);

    }else{
        char to_ok[200];
        snprintf(to_ok,sizeof(to_ok),"550 No such user\r\n");
        send(clientSocket,to_ok,strlen(to_ok),0);
    	return;
    }
    // char buf[100];
    rec_bytes=recv(clientSocket,buf,100,0);
    buf[rec_bytes]='\0';
    printf("%s\n",buf);
    // printf("%d\n",rec_bytes);
     
    if(strncmp(buf,"DATA",4)==0){
        // printf("354\n");
        char sendd[200];
        snprintf(sendd,sizeof(sendd),"354 Enter mail, end with . on a line by itself\r\n");
        send(clientSocket,sendd,strlen(sendd),0);
    }
    // printf("S: 354 Enter mail, end with "." on a line by itself\n");

    printf("%s\n",filePath);
    FILE *fil = fopen(filePath,"a");
    if(fil==NULL){
        perror("file not opened\n");
    }

    // write the from and to addr to the file if the sender doesnot send them again



    time_t currentTime;
    time(&currentTime);
    struct tm *timeInfo = localtime(&currentTime);
    o=0;
    int z=0;
    while((rec_bytes=recv(clientSocket,buf,100,0))>0){
        int x=0,i=0;
        buf[rec_bytes]='\0';
        printf("%s\n",buf);
        
        // fprintf(fil, "%s", buf);
        char str[200];
        int j=0;
        while(i<rec_bytes){
            str[j]=buf[i];
            j++;
            if(buf[i]=='\n') {
                str[j]='\0';
                j=0;
                o++;
                fprintf(fil, "%s", str);
                if(o==3 && z==0){
                    z++;
                    fprintf(fil, "Received: %d-%02d-%02d:%02d:%02d\n",
                        timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday,
                        timeInfo->tm_hour, timeInfo->tm_min);
                }

            }
            
            if(buf[i]=='.'){
                if(i-1>0){
                    if(buf[i-1]=='\n')
                    {
                        str[j]='\r';
                        str[j+1]='\n';
                        str[j+2]='\0';
                        fprintf(fil, "%s", str);
                        x++;
                        break;
                    }
                }else if(i-1<0)  {
                    str[j]='\r';
                    str[j+1]='\n';
                    str[j+2]='\0';
                    fprintf(fil, "%s", str);
                    x++;
                    break;
                }
            }
            i++;
        }
        if(x) break;
        
    }
    printf("%d\n",o);
    char sendd[200];
    snprintf(sendd,sizeof(sendd),"250 OK Message accepted for delivery\r\n");
    send(clientSocket,sendd,strlen(sendd),0);
    // printf("S: 250 OK Message accepted for delivery\n");
    fclose(fil);

    rec_bytes = recv(clientSocket,buf,100,0);
    buf[rec_bytes]='\0';
    if(strncmp(buf,"QUIT",4)==0){
        char sss[1000];
        snprintf(sss,sizeof(sss),"221 iitkgp.edu closing connection\r\n");
        send(clientSocket,sss,strlen(sss),0);
    }
}

int main(int argc, char *argv[]){
    // printf("DATAA\r");
	if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int my_port=atoi(argv[1]);
    printf("%d port\n",my_port);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(my_port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    // char ipAddress[];
    // inet_ntop(AF_INET, &(serINET_ADDRSTRLENverAddress.sin_addr), ipAddress, INET_ADDRSTRLEN);

    // // Print the IP address
    // printf("IP Address: %s\n", ipAddress);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }
    printf("Binded\n");
    listen(serverSocket,5);


    while(1){
    	struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);

        if (clientSocket == -1) {
            perror("Error accepting connection");
            continue;
        }
        // Fork a child process to handle the client
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error forking process");
            close(clientSocket);
            continue;
        } else if (pid > 0) {
            // Parent process (close the client socket and continue listening)
            close(clientSocket);
            continue;
        } else {
            // Child process (handle the client and exit)
            close(serverSocket);

            char connected[200];
            snprintf(connected,sizeof(connected),"220 <iitkgp.edu> Service Ready\r\n");
            send(clientSocket,connected,strlen(connected),0);
            printf("%s",connected);

            char buf[100];
            int rec_bytes=recv(clientSocket,buf,100,0);
            buf[rec_bytes]='\0';
            char str[6];
            for(int i=0;i<4;i++)
                str[i]=buf[i];
            str[4]='\0';
            if(strcmp(str,"HELO\0")!=0){

            }
            char recack[100];
            snprintf(recack,sizeof(recack),"250 OK Hello kgp\r\n");
            send(clientSocket,recack,strlen(recack),0);
            
            handleClient(clientSocket);

            close(clientSocket);
            exit(EXIT_SUCCESS);
        }
    }
    close(serverSocket);
    return 0;
}