# HTTP Proxy Server with LRU Cache

A high-performance, multi-threaded HTTP proxy server implemented in C with intelligent caching capabilities. This proxy server acts as an intermediary between clients and web servers, providing request forwarding, response caching, and connection management.

## Features

### Core Functionality
- **HTTP/1.0 and HTTP/1.1 Support**: Full compatibility with modern HTTP protocols
- **GET Request Handling**: Processes and forwards HTTP GET requests to remote servers
- **Multi-threaded Architecture**: Concurrent handling of up to 400 client connections
- **Connection Pooling**: Efficient management of client connections using semaphores

### Advanced Caching System
- **LRU Cache Implementation**: Least Recently Used eviction policy for optimal memory usage
- **Configurable Cache Size**: 200MB total cache with 10MB per item limit
- **Thread-Safe Operations**: Mutex-protected cache operations for concurrent access
- **Automatic Cache Management**: Smart eviction when cache reaches capacity limits

### Error Handling & HTTP Compliance
- **Comprehensive Error Responses**: Standard HTTP error codes (400, 403, 404, 500, 501, 505)
- **RFC-Compliant Headers**: Proper HTTP header generation and timestamp formatting
- **Connection Management**: Graceful handling of client disconnections and timeouts

##  Requirements

### System Requirements
- **Operating System**: Linux/Unix-based systems
- **Compiler**: GCC with C99 support
- **Libraries**: pthread library for multi-threading support
- **Memory**: Minimum 512MB RAM (1GB+ recommended for optimal caching)

### Development Dependencies
- `gcc` compiler
- `make` build system
- `pthread` library (usually included with glibc)
- Optional: `valgrind` for memory debugging
- Optional: `gprof` for performance profiling

##  Installation

### Quick Start
```bash
# Clone or download the project files
# Ensure you have: proxy_server_with_cache.c, proxy_parse.c, proxy_parse.h, Makefile

# Build the proxy server
make

# Run with default configuration
./proxy_server 8080
```

### Build Options
```bash
# Standard build
make all

# Debug build with symbols
make debug

# Optimized release build
make release

# Clean build artifacts
make clean

# Create submission archive
make tar
```

### System Installation (Optional)
```bash
# Install system-wide (requires sudo)
make install

# Remove from system
make uninstall
```

## 🚦 Usage

### Basic Usage
```bash
# Start proxy server on port 8080
./proxy_server 8080

# Start on custom port
./proxy_server 3128
```

### Client Configuration
Configure your web browser or HTTP client to use the proxy:

**Browser Settings:**
- Proxy Type: HTTP
- Proxy Address: `localhost` (or server IP)
- Proxy Port: `8080` (or your chosen port)

**curl Example:**
```bash
# Use proxy for HTTP requests
curl --proxy localhost:8080 http://example.com

# Test proxy functionality
curl --proxy localhost:8080 http://httpbin.org/get
```

### Testing the Cache
```bash
# First request (cached)
curl --proxy localhost:8080 http://example.com

# Second request (served from cache - faster response)
curl --proxy localhost:8080 http://example.com
```

##  Project Structure

```
proxy-server/
├── proxy_server_with_cache.c    # Main proxy server implementation
├── proxy_parse.c                # HTTP request parsing utilities
├── proxy_parse.h                # Header file with parsing functions
├── Makefile                     # Build configuration and targets
├── README.md                    # This documentation file
└── proxy_server                 # Compiled executable (after build)
```

##  Configuration

### Compile-Time Constants
Located in `proxy_server_with_cache.c`:

```c
#define BUFFER_LIMIT 4096              
#define CONNECTION_POOL_SIZE 400       
#define CACHE_TOTAL_SIZE 200*(1<<20)   
#define CACHE_ITEM_LIMIT 10*(1<<20)    
```

### Runtime Parameters
- **Port Number**: Specified as command-line argument
- **Cache Behavior**: Automatic LRU eviction when full
- **Connection Limits**: Enforced via semaphore mechanism

##  Development

### Building for Development
```bash
# Build with debugging symbols
make debug

# Run syntax check
make check-syntax

# Memory leak detection
make memcheck

# Performance profiling
make profile
```

### Code Structure Overview

#### Main Components:
1. **Server Initialization** (`main`): Socket creation, binding, and listening
2. **Client Handler** (`client_handler_thread`): Per-client request processing
3. **Request Forwarding** (`forward_http_request`): HTTP request relay to remote servers
4. **Cache System**: LRU cache with thread-safe operations
5. **Error Handling** (`send_http_error`): HTTP error response generation

#### Threading Model:
- **Main Thread**: Accepts incoming connections
- **Worker Threads**: Handle individual client requests (max 400)
- **Semaphore Control**: Limits concurrent connections
- **Mutex Protection**: Thread-safe cache operations

### Adding Features
To extend functionality:

1. **New HTTP Methods**: Modify request parsing in `client_handler_thread`
2. **Cache Policies**: Update eviction logic in `evict_oldest_item`
3. **Logging**: Add logging calls throughout request processing
4. **Statistics**: Implement counters for cache hits/misses

##  Troubleshooting

### Common Issues

**Port Already in Use:**
```bash
# Check what's using the port
lsof -i :8080

# Kill process using the port
sudo kill -9 <PID>
```

**Permission Denied:**
```bash
# Use port > 1024 for non-root users
./proxy_server 8080

**Connection Refused:**
```bash
# Verify proxy is running
ps aux | grep proxy_server

# Check if port is accessible
telnet localhost 8080
```

### Debug Mode
```bash
# Build debug version
make debug

# Run with verbose output
./proxy_server 8080 2>&1 | tee proxy.log
```



**Built with disappointment and a lot of patience specially working with the berkeley sockets library(i will always hate you), using C and the power of multi-threading**