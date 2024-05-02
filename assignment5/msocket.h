#ifndef M_SOCKET_H


#define M_SOCKET_H

#include <sys/socket.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <arpa/inet.h> 
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <signal.h>


#define TIMEOUT_DURATION 5
#define PROB 0.3
#define SOCK_MTP 1001
#define ENOTBOUND 10
#define MAX_SOCKETS 25



// struct socket_info SOCK_INFO;
struct sock_info{
    int sock_id;
    int port;
    int errrno;
    char IP[16];
};

// rwnd (a structure of the receiver window size along with an  array of message sequence numbers that have been received but not acknowledged).
struct rwnd{
    int size;
    int first;
    int last;
    int valid[5];
    int msg_count;
    int seqnum;
};

// swnd (a structure of the sender window size along with an array of message sequence numbers that have been sent but not acknowledged).
struct swnd{
    int size;
    int first;
    int last;
    int seqnum;
    int msg_count;
    time_t t;
};

struct sm{
    int status;
    int pid;
    int udp_id;
    int port;
    int nospace;
    int closed;
    char IP[16];   
    char recv_buffer[5][1024];
    char send_buffer[10][1024];
    struct rwnd rwnd;
    struct swnd swnd;
};


int m_socket(int domain, int type, int protocol);
int m_bind(int sockfd, const struct sockaddr *src_addr, socklen_t src_addrlen, const struct sockaddr *des_addr, socklen_t des_addrlen);
int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);   
int m_close(int fd);   

#endif