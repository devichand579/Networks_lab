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


void handlePOP3Client(int clientSocket){

//********************* sending connection OK signal **********************************************
	
	char connected[200];
	snprintf(connected,sizeof(connected),"+OK POP3 server ready <iitkgp.edu>\r\n");
    send(clientSocket,connected,strlen(connected),0);
    printf("%s",connected);

    char buf[201];
  
  //********************* recieving user name **********************************************
    
    int rec_bytes=recv(clientSocket,buf,200,0);
    buf[rec_bytes]='\0';
    char user[101];
    if(strncmp(buf,"USER",4)==0){
    	// extracting username from buf
    	int i=5,j=0;
    	while(buf[i+1]!='\n'){
    		user[j++]=buf[i];
    		i++;
    	}
    	user[j]='\0';
    }else{
    	char error[100];
    	snprintf(error,sizeof(error),"-ERR\r\n");
    	send(clientSocket,error,strlen(error),0);
    	return;
    }
    printf("user: %s\n",user);

    FILE *uf1=fopen("user.txt","r");
    if(uf1==NULL){
    	snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
    	perror("Error in opening the file user.txt");
    	exit(EXIT_FAILURE);
    }
    char line[500];
    int o=0;
    while(fgets(line, sizeof(line), uf1)!=NULL){
    	char un[200];
        if (sscanf(line, "%s", un) == 1) {
        	if(strcmp(un,user)==0){
        		o++;
        		break;
        	}
        }
    }
    fclose(uf1);
    if(o){
    	snprintf(buf,sizeof(buf),"+OK User accepted\r\n");
    	printf("%s\n",buf);
    	send(clientSocket,buf,strlen(buf),0);
    }else{
    	char error[100];
    	snprintf(error,sizeof(error),"-ERR\r\n");
    	printf("%s",error);
    	send(clientSocket,error,strlen(error),0);
    	return;
    }
  
  //********************* recieving Pass **********************************************
    
    rec_bytes = recv(clientSocket,buf,200,0);
    buf[rec_bytes]='\0';
    char pass[101];
    if(strncmp(buf,"PASS",4)==0){
    	// extracting password from buf
    	int i=5,j=0;
    	while(buf[i+1]!='\n'){
    		pass[j++]=buf[i];
    		i++;
    	}
    	pass[j]='\0';
    }
    else{
    	snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
    	return;
    }
    printf("pass: %s\n",pass);


  
  //*********************authenticating **********************************************
    
    FILE *uf=fopen("user.txt","r");
    if(uf==NULL){
    	snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
    	perror("Error in opening the file user.txt");
    	exit(EXIT_FAILURE);
    }
    o=0;
    while(fgets(line, sizeof(line), uf)!=NULL){
    	// printf("%s\n",line);
    	// printf("%s\n",user);
    	char un[200];
        if (sscanf(line, "%s", un) == 1) {
        	if(strcmp(un,user)==0){
        		int i=0;
        		while(line[i]!=' '){
        			i++;
        		}
        		i++;
        		char cpass[101];
        		int j=0;
        		while(line[i]!='\n'){
        			cpass[j++]=line[i];
        			i++;
        		}
        		cpass[j]='\0';
        		if(strcmp(pass,cpass)==0){
        			o++;
        			printf("authentication successful\n");
        			break;
        		}
        	}
        }
    }
    fclose(uf);

  
  //********************* authentication failure **********************************************
    
    if(o==0){
    	snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
    	return;
    }

  
  //********************* getting the directory of user **********************************************
    
    char fullPath[500];
    char filePath[1000];
    if (getcwd(fullPath, sizeof(fullPath)) != NULL) {
	        
        snprintf(filePath, sizeof(filePath), "%s/%s/%s", fullPath, user, "mymailbox");
    } else {
        perror("Error getting current working subdirectory");
        snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
    	return;
    }

  
  //********************* locking mailbox **********************************************
    
    int mailbox_fd = open(filePath, O_RDWR);
    if (mailbox_fd == -1) {
        perror("Error opening mailbox file");
        snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
        exit(EXIT_FAILURE);
    }

  
  //********************* Attempt to acquire an exclusive lock on the mailbox
    
    struct flock fl;
    fl.l_type = F_WRLCK;    // Exclusive lock for writing
    fl.l_whence = SEEK_SET; // Start of file
    fl.l_start = 0;         // Offset
    fl.l_len = 0;           // Whole file
    if (fcntl(mailbox_fd, F_SETLK, &fl) == -1) {
        perror("Error acquiring lock on mailbox");
        snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
        exit(EXIT_FAILURE);
    }

//*********************** counting no of messages **************************************************

    FILE *fil = fopen(filePath,"r");
     if(fil==NULL){
        perror("file not opened\n");
        snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
    	return;
    }

    int numofmes=0;
    while(fgets(line, sizeof(line), fil)!=NULL){
    	if(line[0]=='.')
    		numofmes++;
    	printf("%s",line);
    }
    printf("%d\n",numofmes);
    fclose(fil);

//*********************** counting size of messages*****************************************
    

    FILE *fil1 = fopen(filePath,"r");
     if(fil1==NULL){
        perror("file not opened\n");
        snprintf(buf,sizeof(buf),"-ERR\r\n");
    	send(clientSocket,buf,strlen(buf),0);
    	return;
    }

    size_t mesize[numofmes+1];
    int nu=0;
    size_t mss=0;
    while(fgets(line, sizeof(line), fil1)!=NULL){
    	if(line[0]=='.'){
    		nu++;
    		mesize[nu]=mss;
    		mss=0;
    	}
    	mss+=(size_t)strlen(line);
    }
    mss=0;
    for(int i=1;i<=nu;i++){
    	mss+=mesize[i];
    }
    fclose(fil1);

	snprintf(buf,sizeof(buf),"+OK Pass accepted\r\n");
	printf("%s\n",buf);
	send(clientSocket,buf,strlen(buf),0);


//*************************handling the keyword****************************************************

	int del[numofmes+1];
	for(int i=1;i<=numofmes;i++){
		del[i]=0;
	}
	while(1){
		rec_bytes = recv(clientSocket,buf,200,0);
    	buf[rec_bytes]='\0';
    	if(strcmp(buf,"STAT\r\n\0")==0){
    		snprintf(buf,sizeof(buf),"+OK %d %ld\r\n",numofmes,mss);
			printf("%s\n",buf);
			send(clientSocket,buf,strlen(buf),0);
    	}
    	else if(strcmp(buf,"LIST\r\n\0")==0){
    		snprintf(buf,sizeof(buf),"+OK %d messages (%ld octets)\r\n",numofmes,mss);
			printf("%s\n",buf);
			send(clientSocket,buf,strlen(buf),0);
			for(int i=1;i<=numofmes;i++){
				snprintf(buf,sizeof(buf),"%d %ld\r\n",i,mesize[i]);
				printf("%s\n",buf);
				send(clientSocket,buf,strlen(buf),0);
			}
			snprintf(buf,sizeof(buf),".\r\n");
			printf("%s\n",buf);
			send(clientSocket,buf,strlen(buf),0);
    	}
    	else if(strncmp(buf,"RETR",4)==0){
    		int i=5;
    		char num[1000];
    		int j=0;
    		while(buf[i+1]!='\n'){
    			num[j++]=buf[i];
    			i++;
    		}
    		num[j]='\0';
    		int k=atoi(num);
    		

			FILE *fil2 = fopen(filePath,"r");
		    if(fil2==NULL){
		        perror("file not opened\n");
		        snprintf(buf,sizeof(buf),"-ERR\r\n");
		    	send(clientSocket,buf,strlen(buf),0);
		    	return;
		    }
		    int no=1;
		    int m=0;
		    while(fgets(line, sizeof(line), fil2)!=NULL){
		    	if(m==0){
		    		snprintf(buf,sizeof(buf),"+OK %ld octets\r\n",mesize[k]);
					send(clientSocket,buf,strlen(buf),0);
					printf("%s\n",buf);
		    	}
		    	m++;
		    	if(no==k){
					send(clientSocket,line,strlen(line),0);
					printf("%s\n",line);
		    	}
		    	if(line[0]=='.'){
		    		no++;
		    	}
		    	
		    }
		    fclose(fil2);

    	}
    	else if(strncmp(buf,"DELE",4)==0){
    		int i=5;
    		char num[1000];
    		int j=0;
    		while(buf[i+1]!='\n'){
    			num[j++]=buf[i];
    			i++;
    		}
    		num[j]='\0';
    		int k=atoi(num);
    		del[k]=1;
    		snprintf(buf,sizeof(buf),"+OK %d message deleted\r\n",k);
			printf("%s\n",buf);
			send(clientSocket,buf,strlen(buf),0);
    	}
    	else if(strcmp(buf,"RSET\r\n\0")==0){
    		for(int i=1;i<=numofmes;i++){
    			if(del[i]==1)
    				del[i]=0;
    		}
    	}
    	else if(strcmp(buf,"QUIT\r\n\0")==0){
    		break;
    	}
	}


// ********************* Release the lock after the session is completed **********************************
    fl.l_type = F_UNLCK;
    if (fcntl(mailbox_fd, F_SETLK, &fl) == -1) {
        perror("Error releasing lock on mailbox");
        exit(EXIT_FAILURE);
    }

    // Close the mailbox file
    if (close(mailbox_fd) == -1) {
        perror("Error closing mailbox file");
        exit(EXIT_FAILURE);
    }

//*********************** Deleting the marked ones***************************************

    FILE *inp=fopen(filePath,"r");
    if (inp == NULL) {
        perror("Error opening input file");
        return;
    }

    // Open output file for writing
    char filePath1[1000];
    snprintf(filePath1, sizeof(filePath1), "%s/%s/%s", fullPath, user, "output.txt");
    FILE *out = fopen(filePath1, "w");
    if (out == NULL) {
        perror("Error opening output file");
        fclose(inp);
        return;
    }

    int numm=1;
    while(fgets(line, sizeof(line), inp)!=NULL){

    	if(!del[numm]){
    		fputs(line,out);
    	}

    	if(line[0]=='.'){
    		numm++;
    	}
    }
    fclose(inp);
    fclose(out);

    if (remove(filePath) != 0) {
        perror("Error deleting original file");
        return;
    }
    // snprintf(filePath1, sizeof(filePath1), "%s/%s/%s", fullPath, user, "output.txt");
    if(rename(filePath1,filePath)!=0){
    	perror("Error renaming output file");
        return;
    }

}

int main(int argc, char *argv[]){
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
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(my_port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

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

            
            handlePOP3Client(clientSocket);

            close(clientSocket);
            exit(EXIT_SUCCESS);
        }
    }
    close(serverSocket);
    return 0;
}
