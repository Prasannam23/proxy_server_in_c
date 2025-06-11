#include "map.h"
#include <string.h>
#include <stdio.h>

url_map* url_map_create() {
    url_map* m = malloc(sizeof(url_map));
    if (!m) return NULL;

    m->size = 0;
    for (int i = 0; i < CACHE_CAPACITY; ++i) {
        m->entries[i].url = NULL;
        m->entries[i].value = NULL;
    }
    return m;
}

void url_map_put(url_map* m, const char* url, struct cache* value) {
    if (!m || !url) return;

    for (int i = 0; i < m->size; ++i) {
        if (strcmp(m->entries[i].url, url) == 0) {
            m->entries[i].value = value;
            return;
        }
    }

    if (m->size < CACHE_CAPACITY) {
        m->entries[m->size].url = strdup(url);
        m->entries[m->size].value = value;
        m->size++;
    } else {
        fprintf(stderr, "[url_map_put] Cache full. Eviction needed.\n");
    }
}

struct cache* url_map_get(url_map* m, const char* url) {
    if (!m || !url) return NULL;

    for (int i = 0; i < m->size; ++i) {
        if (m->entries[i].url && strcmp(m->entries[i].url, url) == 0)
            return m->entries[i].value;
    }
    return NULL;
}

void url_map_remove(url_map* m, const char* url) {
    if (!m || !url) return;

    for (int i = 0; i < m->size; ++i) {
        if (strcmp(m->entries[i].url, url) == 0) {
            free(m->entries[i].url);
            for (int j = i; j < m->size - 1; ++j)
                m->entries[j] = m->entries[j + 1];

            m->entries[m->size - 1].url = NULL;
            m->entries[m->size - 1].value = NULL;
            m->size--;
            return;
        }
    }
}

void url_map_free(url_map* m) {
    if (!m) return;
    for (int i = 0; i < m->size; ++i) {
        if (m->entries[i].url)
            free(m->entries[i].url);
    }
    free(m);
}
