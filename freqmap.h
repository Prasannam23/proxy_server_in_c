#ifndef FREQMAP_H
#define FREQMAP_H

#include <stdlib.h>
#include "cache.h"  

typedef struct freq_node {
    int frequency;
    struct cache* head;           
    struct freq_node* next;
} freq_node;

typedef struct {
    freq_node* head;
} freq_map;

freq_map* freq_map_create();
void      freq_map_add(freq_map* f, struct cache* c);
void      freq_map_remove(freq_map* f, struct cache* c);
struct cache* freq_map_evict(freq_map* f);
void      freq_map_free(freq_map* f);

#endif
