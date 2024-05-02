#include "msocket.h"

struct sm *SM;
struct sock_info *SOCK_INFO;
int shm_id_sm,shm_id_sock;

int k1=1,k2=2,k3=3,k4=4,k5=5,k6=6;
struct sembuf pop, vop ;

#define P(s) semop(s, &pop, 1)  // wait
#define V(s) semop(s, &vop, 1)  // signal

int sem1,sem2;
int mtx_R,mtx_S; // mutex for R and S

void attach()
{
    // Attach shared memory and semaphores sem1 ans sem2
    shm_id_sm = shmget(k1, 25*sizeof(struct sm), 0666);
    if (shm_id_sm == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    SM = (struct sm *)shmat(shm_id_sm, NULL, 0);
    if (SM == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

     shm_id_sock = shmget(k2, sizeof(struct sock_info), 0666);
    if (shm_id_sock == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    SOCK_INFO = shmat(shm_id_sock, NULL, 0);
    if (SOCK_INFO == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

    sem1 = semget(k3, 1, 0666);
    sem2 = semget(k4, 1, 0666);

    mtx_R = semget(k5, 1, 0666);
    mtx_S = semget(k6, 1, 0666);   
}

int m_socket(int domain, int type, int protocol) {

  // atach shared memory and semaphores
    attach();
    
    // Check if the socket type is SOCK_MTP
    if (type != SOCK_MTP) {
        errno = EINVAL;
        //deattach the memory
        shmdt(SM);
        shmdt(SOCK_INFO);
        return -1;
    }
   
    for(int i = 0;i<MAX_SOCKETS;i++){
        if(SM[i].status == 0){
            // signals on sem1
            V(sem1);
            // waits on sem2
            P(sem2);
            if(SOCK_INFO->sock_id==-1){
                // set errno
                errno = SOCK_INFO->errrno;
                SOCK_INFO->sock_id = 0;
                SOCK_INFO->port=0;
                memset(SOCK_INFO->IP,0,16);
                SOCK_INFO->errrno = 0;
                //deattach the memory
                shmdt(SM);
                shmdt(SOCK_INFO);
                return -1;
            }

            SM[i].status = 1;
            SM[i].udp_id = SOCK_INFO->sock_id;
            SM[i].pid = getpid();
            SOCK_INFO->sock_id = 0;
            SOCK_INFO->port=0;
            memset(SOCK_INFO->IP,0,16);
            SOCK_INFO->errrno = 0;
            //deattach the memory
            shmdt(SM);
            shmdt(SOCK_INFO);
            return i;
        }
    }
    errno = ENOBUFS;
    //deattach the memory
    shmdt(SM);
    shmdt(SOCK_INFO);
    return -1;

}
 // parameters are source IP and port number and destination IP and port number
int m_bind(int sockfd, const struct sockaddr *src_addr, socklen_t src_addrlen, const struct sockaddr *des_addr, socklen_t des_addrlen)
{

    //attach shared memory and semaphores
    attach();
    char ip[16];
    struct sockaddr_in *addr_in = (struct sockaddr_in *)src_addr;
    SOCK_INFO->sock_id = SM[sockfd].udp_id;
    SOCK_INFO->port = ntohs(addr_in->sin_port);
    strcpy(ip,inet_ntoa(addr_in->sin_addr));
    strcpy(SOCK_INFO->IP,ip);

    
    //signal sem1
    V(sem1);
    //wait on sem2
    P(sem2);
    if(SOCK_INFO->sock_id==-1){
        // set errno
        errno = SOCK_INFO->errrno;
        SOCK_INFO->sock_id = 0;
        SOCK_INFO->port=0;
        memset(SOCK_INFO->IP,0,16);
        SOCK_INFO->errrno = 0;
        //deattach the memory 
        shmdt(SM);
        shmdt(SOCK_INFO);
        return -1;
    }
    struct sockaddr_in *addr_out = (struct sockaddr_in *)des_addr;
    SM[sockfd].port = ntohs(addr_out->sin_port);
    strcpy(SM[sockfd].IP,inet_ntoa(addr_out->sin_addr));
    SOCK_INFO->sock_id = 0;
    SOCK_INFO->port=0;
    memset(SOCK_INFO->IP,0,16);
    SOCK_INFO->errrno = 0;

    //deattach the memory 
    shmdt(SM);
    shmdt(SOCK_INFO);

    return 0;
}

int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {

    // attach shared memory and semaphores
    attach();

    int dest_port = ntohs(((struct sockaddr_in *)dest_addr)->sin_port);
    char *dest_ip = inet_ntoa(((struct sockaddr_in *)dest_addr)->sin_addr);
    // wait on mtx_S
    P(mtx_S);
    if(SM[sockfd].port == dest_port && strcmp(SM[sockfd].IP,dest_ip)==0){
           // check if the remaining space in the send buffer is greater than or equal to the length of the message to be sent
           int x = SM[sockfd].swnd.last;
            if(SM[sockfd].swnd.msg_count<10){
                strcpy(SM[sockfd].send_buffer[x],(char *)buf);
                SM[sockfd].send_buffer[x][1023]='\0';
                int len = strlen(SM[sockfd].send_buffer[x]);
                SM[sockfd].swnd.last = (SM[sockfd].swnd.last+1)%10;
                SM[sockfd].swnd.msg_count++;
                //signal on mtx_S
                V(mtx_S);
                //deattach the memory
                shmdt(SM);
                return len;
            }
            else{
                // set errno
                errno = ENOBUFS;
                //signal on mtx_S
                V(mtx_S);
                //deattach the memory
                shmdt(SM);
                return -1;
            }
        }
    //signal on mtx_S
    V(mtx_S);
    errno = ENOTBOUND;

    //deattach the memory
    shmdt(SM);
    return -1;
}

int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {

    // attach shared memory and semaphores
    attach();


    // wait on mtx_R
    P(mtx_R);
    if(SM[sockfd].rwnd.msg_count>0){
        int x = SM[sockfd].rwnd.first;
        strcpy((char *)buf,SM[sockfd].recv_buffer[x]);
        SM[sockfd].rwnd.first = (SM[sockfd].rwnd.first+1)%5;
        SM[sockfd].rwnd.msg_count--;
        SM[sockfd].rwnd.valid[x] = 0;
        SM[sockfd].nospace = 0;
        SM[sockfd].rwnd.size++;
        //signal on mtx_R
        V(mtx_R);
        //deattach the memory
        shmdt(SM);
        return strlen((char *)buf);
    }
   
    // set errno
    errno = ENOMSG;
    //signal on mtx_R
     V(mtx_R);

    //deattach the memory
    shmdt(SM);
     return -1;



}

int m_close(int fd) {
    
    //attach the shared memory
    attach();


    // wait on mtx_S   
    P(mtx_S);
    if(SM[fd].closed == 1){
        //signal on mtx_S
        V(mtx_S);
        //deattach the memory
        shmdt(SM);
        return -1;
    }
    SM[fd].closed = 1;
    //signal on mtx_S
    V(mtx_S);
    //deattach the memory
    shmdt(SM);
    return 0;
}
