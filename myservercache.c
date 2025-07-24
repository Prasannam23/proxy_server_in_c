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
#include <time.h>

// ok i should have written this at the top but i am writing it here. The question is why is i am building this 
// and the answer is only for you my friend if you have actually decided to go through the code .
// so i want my own request and response mechanism which gives me response. So friend grind and code well and build this
// for sending your requests and guess what it gives you responses unlike that one arrogant girl who just ghosted you


#define BUFFER_LIMIT 4096           // Maximum request/response buffer size
#define CONNECTION_POOL_SIZE 400    // Maximum concurrent client connections
#define CACHE_TOTAL_SIZE 200*(1<<20)     // Total cache memory allocation (200MB)
#define CACHE_ITEM_LIMIT 10*(1<<20)      // Maximum size per cached item (10MB)

typedef struct cache_node cache_node;

struct cache_node {
    char* response_data;        // HTTP response content from remote server
    int data_length;           // Size of stored response data
    char* request_url;         // Original client request URL (cache key)
    time_t access_timestamp;   // Last access time for LRU tracking
    cache_node* next_node;     // Linked list pointer for cache traversal
};

cache_node* search_cache(char* url);
int insert_cache_item(char* response, int response_size, char* url);
void evict_oldest_item();

int server_port = 8080;                    // Default proxy listening port
int proxy_main_socket;                     // Main proxy server socket descriptor
pthread_t client_threads[CONNECTION_POOL_SIZE];  // Thread pool for handling clients
sem_t connection_semaphore;                // Semaphore to limit concurrent connections
pthread_mutex_t cache_mutex;              // Mutex for thread-safe cache operations
cache_node* cache_head;                   // Head pointer of cache linked list
int current_cache_size;                   // Current total size of cached data


int send_http_error(int client_socket, int error_code) {
    char error_response[1024];
    char timestamp_buffer[50];
    time_t current_time = time(0);
    
    struct tm time_data = *gmtime(&current_time);
    strftime(timestamp_buffer, sizeof(timestamp_buffer), "%a, %d %b %Y %H:%M:%S %Z", &time_data);
    
    
    switch(error_code) {
        case 400:
            snprintf(error_response, sizeof(error_response), 
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Length: 95\r\n"
                "Connection: keep-alive\r\n"
                "Content-Type: text/html\r\n"
                "Date: %s\r\n"
                "Server: ProxyServer/1.0\r\n\r\n"
                "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n"
                "<BODY><H1>400 Bad Request</H1>\n</BODY></HTML>", 
                timestamp_buffer);
            printf("Sending 400 Bad Request response\n");
            break;
            
        case 403:
            snprintf(error_response, sizeof(error_response), 
                "HTTP/1.1 403 Forbidden\r\n"
                "Content-Length: 112\r\n"
                "Content-Type: text/html\r\n"
                "Connection: keep-alive\r\n"
                "Date: %s\r\n"
                "Server: ProxyServer/1.0\r\n\r\n"
                "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n"
                "<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", 
                timestamp_buffer);
            printf("Sending 403 Forbidden response\n");
            break;
            
        case 404:
            snprintf(error_response, sizeof(error_response), 
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 91\r\n"
                "Content-Type: text/html\r\n"
                "Connection: keep-alive\r\n"
                "Date: %s\r\n"
                "Server: ProxyServer/1.0\r\n\r\n"
                "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
                "<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", 
                timestamp_buffer);
            printf("Sending 404 Not Found response\n");
            break;
            
        case 500:
            snprintf(error_response, sizeof(error_response), 
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Length: 115\r\n"
                "Connection: keep-alive\r\n"
                "Content-Type: text/html\r\n"
                "Date: %s\r\n"
                "Server: ProxyServer/1.0\r\n\r\n"
                "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n"
                "<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", 
                timestamp_buffer);
            break;
            
        case 501:
            snprintf(error_response, sizeof(error_response), 
                "HTTP/1.1 501 Not Implemented\r\n"
                "Content-Length: 103\r\n"
                "Connection: keep-alive\r\n"
                "Content-Type: text/html\r\n"
                "Date: %s\r\n"
                "Server: ProxyServer/1.0\r\n\r\n"
                "<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>\n"
                "<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", 
                timestamp_buffer);
            printf("Sending 501 Not Implemented response\n");
            break;
            
        case 505:
            snprintf(error_response, sizeof(error_response), 
                "HTTP/1.1 505 HTTP Version Not Supported\r\n"
                "Content-Length: 125\r\n"
                "Connection: keep-alive\r\n"
                "Content-Type: text/html\r\n"
                "Date: %s\r\n"
                "Server: ProxyServer/1.0\r\n\r\n"
                "<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n"
                "<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", 
                timestamp_buffer);
            printf("Sending 505 HTTP Version Not Supported response\n");
            break;
            
        default:
            return -1;
    }
    
    send(client_socket, error_response, strlen(error_response), 0);
    return 1;
}


int establish_remote_connection(char* target_host, int target_port) {
   
    int remote_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(remote_socket_fd < 0) {
        printf("Failed to create socket for remote connection\n");
        return -1;
    }
    
   
    struct hostent *host_info = gethostbyname(target_host);
    if(host_info == NULL) {
        fprintf(stderr, "DNS resolution failed for host: %s\n", target_host);
        return -1;
    }
    
   
    struct sockaddr_in remote_address;
    bzero((char*)&remote_address, sizeof(remote_address));
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(target_port);
    bcopy((char *)host_info->h_addr, (char *)&remote_address.sin_addr.s_addr, host_info->h_length);
    
   
    if(connect(remote_socket_fd, (struct sockaddr*)&remote_address, (socklen_t)sizeof(remote_address)) < 0) {
        fprintf(stderr, "Connection to remote server failed\n");
        return -1;
    }
    
    return remote_socket_fd;
}


int validate_http_version(char *version_string) {
    int version_status = -1;
    
    if(strncmp(version_string, "HTTP/1.1", 8) == 0) {
        version_status = 1;
    }
    else if(strncmp(version_string, "HTTP/1.0", 8) == 0) {
        version_status = 1;  
    }
    else {
        version_status = -1;  
    }
    
    return version_status;
}


int forward_http_request(int client_socket_fd, ParsedRequest *parsed_req, char *original_request) {
    char *request_buffer = (char*)malloc(sizeof(char) * BUFFER_LIMIT);
    
    strcpy(request_buffer, "GET ");
    strcat(request_buffer, parsed_req->path);
    strcat(request_buffer, " ");
    strcat(request_buffer, parsed_req->version);
    strcat(request_buffer, "\r\n");
    
    size_t request_length = strlen(request_buffer);
    
    if(ParsedHeader_set(parsed_req, "Connection", "close") < 0) {
        printf("Failed to set Connection header\n");
    }
    
    if(ParsedHeader_get(parsed_req, "Host") == NULL) {
        if(ParsedHeader_set(parsed_req, "Host", parsed_req->host) < 0) {
            printf("Failed to set Host header\n");
        }
    }
    
    if(ParsedRequest_unparse_headers(parsed_req, request_buffer + request_length, 
                                   (size_t)BUFFER_LIMIT - request_length) < 0) {
        printf("Header serialization failed - proceeding without headers\n");
    }
    
    
    int target_server_port = 80;
    if(parsed_req->port != NULL) {
        target_server_port = atoi(parsed_req->port);
    }
    // we are in the middle of the project and i swear i hate this library i
    // hate using berkeley sockets lekin ghar chalane ke liye karna padd raha hai
   
    int remote_connection = establish_remote_connection(parsed_req->host, target_server_port);
    if(remote_connection < 0) {
        free(request_buffer);
        return -1;
    }
    
    
    int bytes_transmitted = send(remote_connection, request_buffer, strlen(request_buffer), 0);
    bzero(request_buffer, BUFFER_LIMIT);
    
   
    bytes_transmitted = recv(remote_connection, request_buffer, BUFFER_LIMIT-1, 0);
    
 
    char *response_accumulator = (char*)malloc(sizeof(char) * BUFFER_LIMIT);
    int accumulator_capacity = BUFFER_LIMIT;
    int accumulator_index = 0;
    
    // Relay response data from server to client while accumulating for cache
    while(bytes_transmitted > 0) {
        
        bytes_transmitted = send(client_socket_fd, request_buffer, bytes_transmitted, 0);
        
        for(int byte_idx = 0; byte_idx < bytes_transmitted/sizeof(char); byte_idx++) {
            response_accumulator[accumulator_index] = request_buffer[byte_idx];
            accumulator_index++;
        }
        
        accumulator_capacity += BUFFER_LIMIT;
        response_accumulator = (char*)realloc(response_accumulator, accumulator_capacity);
        
        if(bytes_transmitted < 0) {
            perror("Error relaying data to client\n");
            break;
        }
        
        bzero(request_buffer, BUFFER_LIMIT);
        bytes_transmitted = recv(remote_connection, request_buffer, BUFFER_LIMIT-1, 0);
    }
    
    response_accumulator[accumulator_index] = '\0';
    free(request_buffer);
    insert_cache_item(response_accumulator, strlen(response_accumulator), original_request);
    printf("Request processing completed\n");
    free(response_accumulator);
    
    close(remote_connection);
    return 0;
}


void* client_handler_thread(void* socket_descriptor) {
    sem_wait(&connection_semaphore);
    
    int semaphore_value;
    sem_getvalue(&connection_semaphore, &semaphore_value);
    printf("Active connections: %d\n", CONNECTION_POOL_SIZE - semaphore_value);
    
    int* socket_ptr = (int*)(socket_descriptor);
    int client_fd = *socket_ptr;
    int bytes_received, request_length;
    
    char *request_buffer = (char*)calloc(BUFFER_LIMIT, sizeof(char));
    
    bzero(request_buffer, BUFFER_LIMIT);
    bytes_received = recv(client_fd, request_buffer, BUFFER_LIMIT, 0);
    
   
    while(bytes_received > 0) {
        request_length = strlen(request_buffer);
        // Look for HTTP request terminator
        if(strstr(request_buffer, "\r\n\r\n") == NULL) {
            bytes_received = recv(client_fd, request_buffer + request_length, 
                                BUFFER_LIMIT - request_length, 0);
        }
        else {
            break;  // Complete request received
        }
    }
    
    
    char *request_copy = (char*)malloc(strlen(request_buffer) * sizeof(char) + 1);
    for(int i = 0; i < strlen(request_buffer); i++) {
        request_copy[i] = request_buffer[i];
    }
   
    struct cache_node* cached_response = search_cache(request_copy);
    
    if(cached_response != NULL) {
    
        int response_size = cached_response->data_length/sizeof(char);
        int position = 0;
        char chunk_buffer[BUFFER_LIMIT];
        
        while(position < response_size) {
            bzero(chunk_buffer, BUFFER_LIMIT);
            for(int i = 0; i < BUFFER_LIMIT && position < response_size; i++) {
                chunk_buffer[i] = cached_response->response_data[position];
                position++;
            }
            send(client_fd, chunk_buffer, BUFFER_LIMIT, 0);
        }
        printf("Response served from cache\n");
    }
    else if(bytes_received > 0) {
    
        request_length = strlen(request_buffer);
        
        // Request parsing mere dosto kahi jaiyega nahi warna yeh request waste ho jayegi kaash usne bhi meri request accept ki hoti
        // unlucky me got my self ghosted 
        ParsedRequest* http_request = ParsedRequest_create();
        
        if(ParsedRequest_parse(http_request, request_buffer, request_length) < 0) {
            printf("HTTP request parsing failed\n");
        }
        else {
            bzero(request_buffer, BUFFER_LIMIT);
            
        
            if(!strcmp(http_request->method, "GET")) {
                // Validate request components and HTTP version
                if(http_request->host && http_request->path && 
                   (validate_http_version(http_request->version) == 1)) {
                    
                    bytes_received = forward_http_request(client_fd, http_request, request_copy);
                    if(bytes_received == -1) {
                        send_http_error(client_fd, 500);
                    }
                }
                else {
                    send_http_error(client_fd, 500);
                }
            }
            else {
                printf("Unsupported HTTP method: %s\n", http_request->method);
            }
        }
        
        ParsedRequest_destroy(http_request);
    }
    else if(bytes_received < 0) {
        perror("Error receiving data from client\n");
    }
    else if(bytes_received == 0) {
        printf("Client disconnected\n");
    }
    
    
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    free(request_buffer);
    

    sem_post(&connection_semaphore);
    
    sem_getvalue(&connection_semaphore, &semaphore_value);
    printf("Connection released. Available slots: %d\n", semaphore_value);
    free(request_copy);
    return NULL;
}


 // Main server function - initializes proxy server and handles incoming connections
 
int main(int argc, char * argv[]) {
    int client_socket_fd, client_address_length;
    struct sockaddr_in server_address, client_address;
    
    // Initialized semaphore for connection limiting and mutex for cache
    sem_init(&connection_semaphore, 0, CONNECTION_POOL_SIZE);
    pthread_mutex_init(&cache_mutex, NULL);
    
    // Parse command line arguments for port number i dont even know if even need this 
    if(argc >= 2) {
        server_port = atoi(argv[1]);
    }
    else {
        printf("Usage: %s <port_number>\n", argv[0]);
        exit(1);
    }
    
    printf("Initializing proxy server on port: %d\n", server_port);
    
    t
    proxy_main_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_main_socket < 0) {
        perror("Socket creation failed\n");
        exit(1);
    }
    
    // Enable socket address reuse
    int socket_reuse = 1;
    if(setsockopt(proxy_main_socket, SOL_SOCKET, SO_REUSEADDR, 
                  (const char*)&socket_reuse, sizeof(socket_reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed\n");
    }
    
    
    bzero((char*)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    
    
    if(bind(proxy_main_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Port binding failed - port may be in use\n");
        exit(1);
    }
    printf("Socket bound to port: %d\n", server_port);
    
    
    int listen_result = listen(proxy_main_socket, CONNECTION_POOL_SIZE);
    if(listen_result < 0) {
        perror("Socket listen failed\n");
        exit(1);
    }
    
    int thread_counter = 0;
    int client_socket_descriptors[CONNECTION_POOL_SIZE];
   
    while(1) {
        bzero((char*)&client_address, sizeof(client_address));
        client_address_length = sizeof(client_address);
        
        client_socket_fd = accept(proxy_main_socket, (struct sockaddr*)&client_address, 
                                (socklen_t*)&client_address_length);
        
        if(client_socket_fd < 0) {
            fprintf(stderr, "Client connection acceptance failed\n");
            exit(1);
        }
        else {
            client_socket_descriptors[thread_counter] = client_socket_fd;
        }
        
       
        struct sockaddr_in* client_info = (struct sockaddr_in*)&client_address;
        struct in_addr client_ip = client_info->sin_addr;
        char ip_string[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_ip, ip_string, INET_ADDRSTRLEN);
        
        printf("New client connected - Port: %d, IP: %s\n", 
               ntohs(client_address.sin_port), ip_string);
        
        
        pthread_create(&client_threads[thread_counter], NULL, client_handler_thread, 
                      (void*)&client_socket_descriptors[thread_counter]);
        thread_counter++;
    }
    
    close(proxy_main_socket);
    return 0;
}


  //to search for existing data in the cache
  //Updates LRU timestamp if found and returns cache node pointer with the susequent result 
 
cache_node* search_cache(char* target_url) {
    cache_node* current_node = NULL;
    int mutex_status = pthread_mutex_lock(&cache_mutex);
    printf("Cache search lock acquired (status: %d)\n", mutex_status);
    
    if(cache_head != NULL) {
        current_node = cache_head;
        while(current_node != NULL) {
            if(!strcmp(current_node->request_url, target_url)) {
                printf("Cache hit - URL found\n");
                printf("Previous access time: %ld\n", current_node->access_timestamp);
                
               
                current_node->access_timestamp = time(NULL);
                printf("Updated access time: %ld\n", current_node->access_timestamp);
                break;
            }
            current_node = current_node->next_node;
        }
    }
    else {
        printf("Cache miss - URL not found\n");
    }
    
    mutex_status = pthread_mutex_unlock(&cache_mutex);
    printf("Cache search lock released (status: %d)\n", mutex_status);
    
    return current_node;
}


   //Removes least recently used cache item to free up space and resources
  //Implements LRU eviction policy by finding node with oldest timestamp
 
void evict_oldest_item() {
    cache_node *previous_node;
    cache_node *current_node;
    cache_node *lru_node;
    
    int mutex_status = pthread_mutex_lock(&cache_mutex);
    printf("Cache eviction lock acquired (status: %d)\n", mutex_status);
    
    if(cache_head != NULL) {
        // Find node with least recent access time
        for(current_node = cache_head, previous_node = cache_head, lru_node = cache_head; 
            current_node->next_node != NULL; current_node = current_node->next_node) {
            
            if(((current_node->next_node)->access_timestamp) < (lru_node->access_timestamp)) {
                lru_node = current_node->next_node;
                previous_node = current_node;
            }
        }
        
       
        if(lru_node == cache_head) {
            cache_head = cache_head->next_node;
        } else {
            previous_node->next_node = lru_node->next_node;
        }
        
       
        current_cache_size = current_cache_size - (lru_node->data_length) - 
                           sizeof(cache_node) - strlen(lru_node->request_url) - 1;
        
        
        free(lru_node->response_data);
        free(lru_node->request_url);
        free(lru_node);
    }
    
    mutex_status = pthread_mutex_unlock(&cache_mutex);
    printf("Cache eviction lock released (status: %d)\n", mutex_status);
}


   //Adds new response to cache with least recently used algo tracking 
 // this function will do cache size management and eviction of old items when needed in the cache 
 
int insert_cache_item(char* response_data, int data_size, char* request_url) {
    int mutex_status = pthread_mutex_lock(&cache_mutex);
    printf("Cache insertion lock acquired (status: %d)\n", mutex_status);
    
    int total_item_size = data_size + 1 + strlen(request_url) + sizeof(cache_node);
    
    
    if(total_item_size > CACHE_ITEM_LIMIT) {
        mutex_status = pthread_mutex_unlock(&cache_mutex);
        printf("Cache insertion lock released - item too large (status: %d)\n", mutex_status);
        return 0;
    }
    else {
       
        while(current_cache_size + total_item_size > CACHE_TOTAL_SIZE) {
            evict_oldest_item();
        }
        
       
        cache_node* new_item = (cache_node*)malloc(sizeof(cache_node));
        
        // Allocate and copy response data
        new_item->response_data = (char*)malloc(data_size + 1);
        strcpy(new_item->response_data, response_data);
        
        
        new_item->request_url = (char*)malloc(1 + (strlen(request_url) * sizeof(char)));
        strcpy(new_item->request_url, request_url);
        
       
        new_item->access_timestamp = time(NULL);
        new_item->data_length = data_size;
        
     
        new_item->next_node = cache_head;
        cache_head = new_item;
        current_cache_size += total_item_size;
        
        mutex_status = pthread_mutex_unlock(&cache_mutex);
        printf("Cache insertion completed (status: %d)\n", mutex_status);
        
        return 1;
    }
    return 0;
}