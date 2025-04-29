

// #include "proxy_parse.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/types.h>

// // For creating sockets, binding, listening, accepting (socket(), bind(), listen(), accept())
// #include <sys/socket.h>
// // For sockaddr_in structure (defines Internet addresses)
// #include <netinet/in.h>
// #include <netdb.h>

// // For functions like htons() (host-to-network short conversion), inet_ntoa() (IP address conversion)

// #include <arpa/inet.h>
// //used for system calls 
// #include <unistd.h>
// #include <fcntl.h>
// #include <time.h>
// #include <sys/wait.h>

// // For error handling, provides errno and perror()

// #include <errno.h>
// #include <pthread.h>
// #include <semaphore.h>
// #include <time.h>
// #define Max_threads 20
// typedef struct cache cache;

// struct chache{0
//     char data ;.
//     int len;
//     char url;
//     time_t time_track_lru;
//     cache* next;
// }


// const cache* find(char* url);

// int add_cache(char* data,int size,char* url)
// void remove_cache();
  

//   int portnumber = 5000;

//   int proxy_socketid;

//   pthread_t tid(Max_threads)
//   sem_t semaphore;
//   pthread_mutex_t lock;

// cache* head 
// int cache_size;

// int main(int argc,char*argv[]){
//     int client_socketid,client_len;
//     struct sockaddr_in server_addr, client_addr;
//     sem_init(&semaphore,0,Max_threads);
//     pthread_mutex_t init(lock,NULL);

//     if(agrv==2){
//         port_number = atoi(argv[1]);


// }
// else{
//     printf("too few arguments")
//     exit(1);
// }

// printf("Starting Proxy Server at port:%d\n",portnumber);
// proxy_socketid = socket(AF_INET,SOCK_STREAM,0)
  
// }
#include "server.h"
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
#include <time.h>

#define struct cache cache;
#define Max_threads 10
struct cache{
    char data ;.
    int len;
    char url;
    time_t time_lru;
    cache* next;
}

 cache* find(char* url);

int add_cache(char* data,int size,char* url)
void remove_cache();
  

  int portnumber = 5000;

  int proxy_socketid;
// have defined the max threads that can be used 
  pthread_t tid(Max_threads)

//to solve the issue with using the concpt of shared resources we need to use semaphor and mutex lock 

  sem_t semaphore;
  pthread_mutex_t lock;

cache* head ;
int cache_buffer_size;

int main(int argc,char*argv[]){
    int client_socketid,client_len;
    struct sockaddr_in server_addr, client_addr;
    sem_init(&semaphore,0,Max_threads);
    pthread_mutex_init(&lock,NULL);

  if (argc == 2) {
        char *endptr;
        long port_number = strtol(argv[1], &endptr, 10);

        // Error checking:
        if (*endptr != '\0') {
            printf("Invalid port number: %s\n", argv[1]);
            return 1;
        }

        printf("Port number is: %ld\n", port_number);
    } else {
        printf("Usage: %s <port_number>\n", argv[0]);
        exit(1)
    }


printf("Starting Proxy Server at port:%d\n",portnumber);
proxy_socketid = socket(AF_INET,SOCK_STREAM,0)
  if (proxy_socketid < 0){
    perror("failed to create a socket\n");
    exit(1);
  }
int reuse = 1;
if(setsocketopt(proxy_socketid,SQL_SOCKET,SQ_REUSEADDR,(const char)&reuse,sizeof(reuse))<0  ){
    perror("setSockOpt failed\n")
}

bzero((char*)&server_addr,sizeof(server_addr));
server_addr.sin_family =AF_INET;
server_addr.sin_port = htons(port_number);
server_addr.sin_addr.s_addr =   INADDR_ANY;

}
if(bind(proxy_socketid,(struct sockaddr*)&server_addr,sizeof(server_addr0)<0)){
    perror("port is not available\n");
    exit(1);
}


