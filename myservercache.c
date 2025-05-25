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
#define MAX_BYTES 4096

typedef struct cache {
    char* data;
    int len;
    char* url;
    time_t time_track_lru;
    struct cache* next;
} cache;

const cache* find(char* url);
int add_cache(char* data, int size, char* url);
void remove_cache();

int portnumber = 5000;
int proxy_socketid;
pthread_t tid[MAX_THREADS];
sem_t semaphore;
pthread_mutex_t lock;
cache* head = NULL;

void* thread_fn(void* socketNew) {
    sem_wait(&semaphore);

    int socket = *((int*)socketNew);
    char *buffer = (char*)calloc(MAX_BYTES, sizeof(char));
    if (!buffer) {
        perror("Buffer allocation failed");
        close(socket);
        sem_post(&semaphore);
        return NULL;
    }

    int bytes_recv = recv(socket, buffer, MAX_BYTES, 0);
    while (bytes_recv > 0 && strstr(buffer, "\r\n\r\n") == NULL) {
        int len = strlen(buffer);
        bytes_recv = recv(socket, buffer + len, MAX_BYTES - len, 0);
    }

    if (bytes_recv > 0) {
        // Create a copy of the request
        char *tempReq = strdup(buffer);

        pthread_mutex_lock(&lock);
        const cache* cached = find(tempReq);
        pthread_mutex_unlock(&lock);

        if (cached != NULL) {
            // Cache hit
            int pos = 0;
            while (pos < cached->len) {
                int chunk = (cached->len - pos < MAX_BYTES) ? (cached->len - pos) : MAX_BYTES;
                send(socket, cached->data + pos, chunk, 0);
                pos += chunk;
            }
            printf("Data served from cache.\n");
        } else {
            // Parse and forward
            ParsedRequest* request = ParsedRequest_create();
            if (ParsedRequest_parse(request, buffer, bytes_recv) == 0) {
                if (strcmp(request->method, "GET") == 0 &&
                    request->host && request->path &&
                    checkHTTPversion(request->version) == 1) {

                    int res = handle_request(socket, request, tempReq);
                    if (res == -1) {
                        sendErrorMessage(socket, 500);
                    }
                } else {
                    sendErrorMessage(socket, 500);
                }
            } else {
                printf("Failed to parse request.\n");
                sendErrorMessage(socket, 400);
            }
            ParsedRequest_destroy(request);
        }
        free(tempReq);
    } else if (bytes_recv < 0) {
        perror("recv failed");
    } else {
        printf("Client disconnected.\n");
    }

    free(buffer);
    shutdown(socket, SHUT_RDWR);
    close(socket);
    sem_post(&semaphore);
    return NULL;
}

int main(int argc, char* argv[]) {
    struct sockaddr_in server_addr, client_addr;
    int client_socketid;
    socklen_t client_len;

    if (argc == 2) {
        portnumber = atoi(argv[1]);
    } else {
        fprintf(stderr, "Usage: %s <portnumber>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Starting Proxy Server on port %d...\n", portnumber);

    sem_init(&semaphore, 0, MAX_THREADS);
    pthread_mutex_init(&lock, NULL);

    proxy_socketid = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_socketid < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(proxy_socketid, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portnumber);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(proxy_socketid, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(proxy_socketid, MAX_THREADS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Proxy server is listening...\n");

    int i = 0;
    while (1) {
        client_len = sizeof(client_addr);
        client_socketid = accept(proxy_socketid, (struct sockaddr*)&client_addr, &client_len);
        if (client_socketid < 0) {
            perror("Accept failed");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        printf("Client connected: IP=%s, Port=%d\n", ip_str, ntohs(client_addr.sin_port));

        pthread_create(&tid[i % MAX_THREADS], NULL, thread_fn, &client_socketid);
        i++;
    }

    close(proxy_socketid);
    return 0;
}
