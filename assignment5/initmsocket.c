#include "msocket.h"


// Shared memory segment
int shm_id_sm,shm_id_sock;
struct sm *SM;
struct sock_info *SOCK_INFO;

int k1=1,k2=2,k3=3,k4=4,k5=5,k6=6;
struct sembuf pop, vop ;

#define P(s) semop(s, &pop, 1)  // wait
#define V(s) semop(s, &vop, 1)  // signal

int sem1,sem2;
int mtx_R,mtx_S; // mutex for R and S


int drop_message()
{
    srand(time(NULL));
    if ((double)rand() / (double)RAND_MAX <  PROB)
    {
        return 1;
    }
    return 0;
}

void initialisation()
{
    // Create shared memory segment
    shm_id_sm = shmget(k1, 25*sizeof(struct sm), IPC_CREAT | 0666);
    if (shm_id_sm == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    SM = (struct sm *)shmat(shm_id_sm, NULL, 0);
    if (SM == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    // initilaize all the shared memory
    for(int i=0;i<MAX_SOCKETS;i++){
        SM[i].status = 0;
        SM[i].pid = 0;
        SM[i].udp_id = 0;
        SM[i].port = 0;
        memset(SM[i].IP,0,16);
        SM[i].nospace = 0;
        SM[i].closed = 0;
        SM[i].rwnd.size = 5;
        SM[i].rwnd.first = 0;
        SM[i].rwnd.last = 0;
        SM[i].rwnd.valid[0] = 0;
        SM[i].rwnd.valid[1] = 0;
        SM[i].rwnd.valid[2] = 0;
        SM[i].rwnd.valid[3] = 0;
        SM[i].rwnd.valid[4] = 0;
        SM[i].rwnd.msg_count = 0;
        SM[i].rwnd.seqnum = 1;
        SM[i].swnd.size = 5;
        SM[i].swnd.first = 0;
        SM[i].swnd.last = 0;
        SM[i].swnd.seqnum = 1;
        SM[i].swnd.msg_count = 0;
        SM[i].swnd.t = 0;

    }
    shm_id_sock = shmget(k2, sizeof(struct sock_info), IPC_CREAT | 0666);
    if (shm_id_sock == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    SOCK_INFO = shmat(shm_id_sock, NULL, 0);
    if (SOCK_INFO == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    SOCK_INFO->sock_id = 0;
    SOCK_INFO->port = 0;
    memset(SOCK_INFO->IP,0,16);
    SOCK_INFO->errrno = 0;

    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

    sem1 = semget(k3, 1, 0666|IPC_CREAT);
    sem2 = semget(k4, 1, 0666|IPC_CREAT);
    mtx_R = semget(k5, 1, 0666|IPC_CREAT);
    mtx_S = semget(k6, 1, 0666|IPC_CREAT);

    semctl(sem1, 0, SETVAL, 0);
    semctl(sem2, 0, SETVAL, 0);
    semctl(mtx_R, 0, SETVAL, 1);
    semctl(mtx_S, 0, SETVAL, 1);

}

void *thread_R(void *arg) {
    printf(" Thread R created\n");
    fd_set read_fds;
    int max_fd = -1;

    // Initialize file descriptor sets
    FD_ZERO(&read_fds);
    int dup_nospace[25]={0};

    while (1) {
        // Add all active UDP sockets to the read file descriptor set
        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (SM[i].status == 1) {
                FD_SET(SM[i].udp_id, &read_fds);
                max_fd = (SM[i].udp_id> max_fd) ? SM[i].udp_id : max_fd;
            }
        }

        // Set timeout duration
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_DURATION;
        timeout.tv_usec = 0;

        // Select on all UDP sockets
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (activity == -1) {
            continue;
        } else {
            for(int i=0 ; i< MAX_SOCKETS; i++){
                //wait for mtx_R
                P(mtx_R);
               if(FD_ISSET(SM[i].udp_id, &read_fds) && SM[i].status == 1){
                    struct sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    char buffer[1034];
                    memset(buffer,0,1034);
                    int x = recvfrom(SM[i].udp_id, buffer, 1034, 0, (struct sockaddr *)&addr, &addr_len);
                    int d = drop_message();
                    if(d==1){
                        printf("Packet dropped\n");
                        //signal of mtx_R
                        V(mtx_R);
                        continue;
                    }
                    if(x==-1){
                       // error
                       perror("recvfrom");
                       exit(EXIT_FAILURE);
                    }
                    // Check if the received message is an ACK
                    if(buffer[0] == 'A'){
                       int ack_seq,j,wnd_size;
                      sscanf(buffer, "A %d %d", &ack_seq, &wnd_size);
                      printf("ACK recieved sequence number = %d \n",ack_seq);
                     // Check if the received ACK is a duplicate
                     for(j=0;j<SM[i].rwnd.size;j++){
                        int x = (SM[i].swnd.seqnum+j)%16;  
                        if(x == ack_seq){
                            break;
                        }  
                     }
                    if(j!=SM[i].rwnd.size){
                        for(int k=0;k<=j;k++){
                             memset(&SM[i].send_buffer[(SM[i].swnd.first+k)%10],0,1024);
                             SM[i].swnd.msg_count--;
                             SM[i].swnd.first = (SM[i].swnd.first+1)%10;
                             SM[i].swnd.seqnum = (SM[i].swnd.seqnum+1)%16;
                        }
                         SM[i].swnd.size = wnd_size;
                        }
                    }
                    else if(buffer[0]=='D'){
                        int wnd_size,z;
                        sscanf(buffer, "D %d %d", &wnd_size,&z);
                        printf("Duplicate Recieved , window size = %d\n",wnd_size);
                        SM[i].swnd.size = wnd_size;
                        dup_nospace[z] = 0;
                    }
                    else if(buffer[0]=='M'){
                        int seq;
                        char msg[1024];
                        memset(msg,0,1024);
                        sscanf(buffer, "M %d", &seq);
                        if(seq <= 9){
                            strcpy(msg,buffer+4);
                        }
                        else{
                            strcpy(msg,buffer+5);
                        }
                        printf("Message Recieved ,sequence number = %d length = %d\n",seq,strlen(msg));
                        int j;
                        if(seq==SM[i].rwnd.seqnum){
                            strcpy(SM[i].recv_buffer[SM[i].rwnd.last],msg);
                            SM[i].rwnd.valid[SM[i].rwnd.last] = 1;
                            while(1){
                                SM[i].rwnd.last = (SM[i].rwnd.last+1)%5;
                                SM[i].rwnd.msg_count++;
                                SM[i].rwnd.seqnum = (SM[i].rwnd.seqnum+1)%16; 
                                SM[i].rwnd.size--;  
                                if(SM[i].rwnd.valid[SM[i].rwnd.last]==0){
                                    break;
                                }
                                if(SM[i].rwnd.msg_count==5){
                                    SM[i].nospace = 1;
                                    dup_nospace[i] = 1;
                                    break;
                                }
                            }
                            char ack[1024];
                            memset(ack,0,1024);
                            sprintf(ack,"A %d %d",(SM[i].rwnd.seqnum+15)%16,SM[i].rwnd.size);
                            int x = sendto(SM[i].udp_id, ack, strlen(ack), 0, (struct sockaddr *)&addr, sizeof(addr));
                        }
                        else{
                            for(int j=0;j<SM[i].rwnd.size;j++){
                                int num = (SM[i].rwnd.seqnum+j)%16;
                                if(num == seq && SM[i].rwnd.valid[j]==0){
                                    strcpy(SM[i].recv_buffer[(SM[i].rwnd.last+j)%5],msg);
                                    SM[i].rwnd.valid[j] = 1;
                                    break;
                                }
                            }
                            char ack[1024];
                            memset(ack,0,1024);
                            sprintf(ack,"A %d %d",(SM[i].rwnd.seqnum+15)%16,SM[i].rwnd.size); 
                            int x = sendto(SM[i].udp_id, ack, strlen(ack), 0, (struct sockaddr *)&addr, sizeof(addr));
                        }

                    }
                }
                //handling the case when there is no space in the receiver buffer
                if(dup_nospace[i]==1 && SM[i].nospace==0){
                    printf("Sending Duplicate ACK window size = %d from socket = %d\n",SM[i].rwnd.size,i);
                    char buffer[1024];
                    memset(buffer,0,1024);
                    sprintf(buffer,"D %d %d",SM[i].rwnd.size, i);
                    struct sockaddr_in addr;
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(SM[i].port);
                    addr.sin_addr.s_addr = inet_addr(SM[i].IP);
                    int x = sendto(SM[i].udp_id, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, sizeof(addr));
                    if(x==-1){
                        // error
                        perror("sendto");
                        exit(EXIT_FAILURE);
                    }
                }
                //signal of mtx_R
                V(mtx_R);
            }
        
         }
    }
}

void *thread_S() {
    printf(" Thread S created\n");
   
    while (1) {
        // Sleep for some time (less than T/2)
        sleep((TIMEOUT_DURATION) / 2);
        for (int i = 0; i < MAX_SOCKETS; i++) {
            //wait for mtx_S
            P(mtx_S);
            if (SM[i].status == 1) {
                // Check if the message timeout period (T) is over for the messages sent over any of the active MTP sockets
                time_t current_time;
                time(&current_time);
                if((current_time - SM[i].swnd.t) >= TIMEOUT_DURATION){
                    struct sockaddr_in addr2;
                    addr2.sin_family = AF_INET;
                    addr2.sin_port = htons(SM[i].port);
                    addr2.sin_addr.s_addr = inet_addr(SM[i].IP);
                    for(int j=0;j < SM[i].swnd.size;j++){
                        if(j<SM[i].swnd.msg_count){
                            int p = (SM[i].swnd.first+j)%10;
                            char buffer[1034];
                            memset(buffer,0,1034);
                            sprintf(buffer,"M %d %s",(SM[i].swnd.seqnum+j)%16,SM[i].send_buffer[p]);
                            int x = sendto(SM[i].udp_id, buffer, strlen(buffer), 0, (struct sockaddr *)&addr2, sizeof(addr2));
                            printf("sending messages %d to %d %s from %d\n",SM[i].swnd.seqnum+j,SM[i].port,SM[i].IP,SM[i].udp_id);
                            SM[i].swnd.t = current_time;
                            if(x==-1){
                              // error
                              perror("sendto");
                              exit(EXIT_FAILURE);
                            }
                        }
                    }

                }

            }
            //signal mtx_S
            V(mtx_S);
        }

    }

}

void *thread_G() {
    printf(" Thread G created\n");
    while (1) {
        // Sleep for some time (less than T)
        sleep(TIMEOUT_DURATION/2);
        for (int i = 0; i < MAX_SOCKETS; i++) {
            //wait for mtx_S
            P(mtx_S);
            if (SM[i].status == 1 && SM[i].closed == 0) {
                //check wether the process of the socket is killed or not
                if(kill(SM[i].pid,0)==-1){
                     SM[i].status = 0;
                     SM[i].pid = 0;
                     SM[i].udp_id = 0;
                     SM[i].port = 0;
                     memset(SM[i].IP,0,16);
                     SM[i].nospace = 0;
                     SM[i].closed = 0;
                     SM[i].rwnd.size = 5;
                     SM[i].rwnd.first = 0;
                     SM[i].rwnd.last = 0;
                     SM[i].rwnd.valid[0] = 0;
                     SM[i].rwnd.valid[1] = 0;
                     SM[i].rwnd.valid[2] = 0;
                     SM[i].rwnd.valid[3] = 0;
                     SM[i].rwnd.valid[4] = 0;
                     SM[i].rwnd.msg_count = 0;
                     SM[i].rwnd.seqnum = 1;
                     SM[i].swnd.size = 5;
                     SM[i].swnd.first = 0;
                     SM[i].swnd.last = 0;
                     SM[i].swnd.seqnum = 1;
                     SM[i].swnd.msg_count = 0;
                     SM[i].swnd.t = 0;
                }
            }
            else if(SM[i].status == 1 && SM[i].closed == 1){
                //close the socket
                close(SM[i].udp_id);
                 SM[i].status = 0;
                 SM[i].pid = 0;
                 SM[i].udp_id = 0;
                 SM[i].port = 0;
                 memset(SM[i].IP,0,16);
                 SM[i].nospace = 0;
                 SM[i].closed = 0;
                 SM[i].rwnd.size = 5;
                 SM[i].rwnd.first = 0;
                 SM[i].rwnd.last = 0;
                 SM[i].rwnd.valid[0] = 0;
                 SM[i].rwnd.valid[1] = 0;
                 SM[i].rwnd.valid[2] = 0;
                 SM[i].rwnd.valid[3] = 0;
                 SM[i].rwnd.valid[4] = 0;
                 SM[i].rwnd.msg_count = 0;
                 SM[i].rwnd.seqnum = 1;
                 SM[i].swnd.size = 5;
                 SM[i].swnd.first = 0;
                 SM[i].swnd.last = 0;
                 SM[i].swnd.seqnum = 1;
                 SM[i].swnd.msg_count = 0;
                 SM[i].swnd.t = 0;
            }
            //signal mtx_S
            V(mtx_S);
        }

    }
}




int main() {
    // Initialize shared memory
    initialisation();

    // Create threads R and S
    pthread_t thread_R_id, thread_S_id, thread_G_id;
    pthread_create(&thread_R_id, NULL, thread_R, NULL);
    pthread_create(&thread_S_id, NULL, thread_S, NULL);
    pthread_create(&thread_G_id, NULL, thread_G, NULL);

    while(1){
        //wait on sem1
        P(sem1);
        
        if(SOCK_INFO->sock_id==0 && SOCK_INFO->port==0 && SOCK_INFO->errrno==0){
            // m_socket call
            int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if(sockfd == -1){
                SOCK_INFO->sock_id = -1;
                SOCK_INFO->errrno = errno;
                V(sem2);
                continue;
            }
            printf("Socket created\n");
            SOCK_INFO->sock_id = sockfd;
            V(sem2);
        } 
        else{
            // m_bind call
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(SOCK_INFO->port);
            addr.sin_addr.s_addr =  inet_addr(SOCK_INFO->IP);
            int status = bind(SOCK_INFO->sock_id, (struct sockaddr *)&addr, sizeof(addr));
            if(status == -1){
                SOCK_INFO->sock_id = -1;
                SOCK_INFO->errrno = errno;

                V(sem2);
                continue;
            }
            printf("Socket binded at %d %d %s\n",SOCK_INFO->sock_id,SOCK_INFO->port,SOCK_INFO->IP);
            V(sem2);
        }
    }


    // Main thread waits for all threads to finish
    pthread_join(thread_R_id, NULL);
    pthread_join(thread_S_id, NULL);
    pthread_join(thread_G_id, NULL);

    // Detach and delete shared memory
    shmdt(SM);
    shmctl(shm_id_sm, IPC_RMID, NULL);
    shmdt(SOCK_INFO);
    shmctl(shm_id_sock, IPC_RMID, NULL);

    //Delete the semaphores
    semctl(sem1, 0, IPC_RMID, 0);
    semctl(sem2, 0, IPC_RMID, 0);
    semctl(mtx_R, 0, IPC_RMID, 0);
    semctl(mtx_S, 0, IPC_RMID, 0);

    return 0;
}












