#ifndef MAP_H
#define MAP_H

#include <stdlib.h>
#include "cache.h"  

#define CACHE_CAPACITY 10

typedef struct {
    char* url;
    struct cache* value;
} url_map_entry;

typedef struct {
    url_map_entry entries[CACHE_CAPACITY];
    int size;
} url_map;

url_map* url_map_create();
void     url_map_put(url_map* m, const char* url, struct cache* value);
struct cache* url_map_get(url_map* m, const char* url);
void     url_map_remove(url_map* m, const char* url);
void     url_map_free(url_map* m);

#endif
