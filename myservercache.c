#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_THREADS 10

typedef struct cache {
    char* data;
    int len;
    char* url;
    time_t time_track_lru;
    struct cache* next;
} cache;

// Cache-related function declarations
const cache* find(char* url);
int add_cache(char* data, int size, char* url);
void remove_cache();

// Global variables
int portnumber = 5000;
int proxy_socketid;
pthread_t tid[MAX_THREADS];
sem_t semaphore;
pthread_mutex_t lock;
cache* head = NULL;
int cache_size = 0;

int main(int argc, char* argv[]) {
    int client_socketid;
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;

    // Check arguments
    if (argc == 2) {
        portnumber = atoi(argv[1]);
    } else {
        printf("Usage: %s <portnumber>\n", argv[0]);
        exit(1);
    }

    printf("Starting Proxy Server at port: %d\n", portnumber);

    // Initialize semaphore and mutex
    sem_init(&semaphore, 0, MAX_THREADS);
    pthread_mutex_init(&lock, NULL);

    // Create proxy server socket
    proxy_socketid = socket(AF_INET, SOCK_STREAM, 0);

    if(proxy_socketid<0){
        perror("error in creating a socket");
        exit(1);
    }

    int resuse = 1;

    if(setsockopt(proxy_socketid,SQL_SOCKET,SO_REUSEADDR,(const char*)&reuse,sizeof(reuse))){
        perror("setSockOpt not working");
    }

    bzero((char*)&server_addr,sizeof(server_addr))

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portnumber);
    server_addr.sin_addr.s_addr= INADDR_ANY;

    if(bind(proxy_socketid,(struct sockaddr*)&server_addr,sizeof(server_addr)<0)){
      perror("Port is not available");
      exit(1);
    }

    printf("Binding on port %d\n",port_number);

    int listening_status = listen(proxy_socketid,MAX_THREADS);

    if(listening_status<0){
        perror("Error in listening\n");
        exit(1)
    }
  int i = 0 ;
  int Connected_socketid[MAX_THREADS];

  while(1){
    bzero((char*)&client_addr,sizeof(client_addr));
    clientlenght = sizeof(client_addr);
    client_socketid = accept(proxy_socketid,(struct sockaddr *)&client_addr,(socklen_t*)&clientlenght);

    if (client_socketid<0){
        printf("couldn't connect");
        exit(1);
    }
    else{
        Connected_socketid[i] = client_socketid;
    }

    struct sockaddr_in * client_pt = (struct sockaddr_in *)&client_addr;
    struct in_addr ip_addr = client_pt->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&ip_addr,str,INET_ADDRSTRLEN);
    printf("client is connected with the port number %d and ip address is %s\n",ntohs(client_addr.sin_port));

  }



  //htons converts the computers native host byte order to network byte order
// sockaddr_in in c is a struct used for storing address information for an IPv4 socket 

}