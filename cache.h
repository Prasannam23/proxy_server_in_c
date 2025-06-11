#ifndef CACHE_H
#define CACHE_H

#include <time.h>

typedef struct cache {
    char* data;
    int len;
    char* url;
    int frequency;
    time_t time_track;
    struct cache* prev;
    struct cache* next;
} cache;

#endif
