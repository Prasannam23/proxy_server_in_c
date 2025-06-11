#include "freqmap.h"
#include <stdio.h>

freq_map* freq_map_create() {
    freq_map* f = malloc(sizeof(freq_map));
    if (!f) return NULL;
    f->head = NULL;
    return f;
}

static freq_node* find_or_create_freq_node(freq_map* f, int frequency) {
    freq_node *prev = NULL, *curr = f->head;

    while (curr && curr->frequency < frequency) {
        prev = curr;
        curr = curr->next;
    }

    if (curr && curr->frequency == frequency)
        return curr;

    freq_node* new_node = malloc(sizeof(freq_node));
    new_node->frequency = frequency;
    new_node->head = NULL;
    new_node->next = curr;

    if (prev)
        prev->next = new_node;
    else
        f->head = new_node;

    return new_node;
}

void freq_map_add(freq_map* f, struct cache* c) {
    if (!f || !c) return;

    freq_node* node = find_or_create_freq_node(f, c->frequency);
    c->next = node->head;
    if (node->head)
        node->head->prev = c;
    node->head = c;
    c->prev = NULL;
}

void freq_map_remove(freq_map* f, struct cache* c) {
    if (!f || !c) return;

    freq_node* node = f->head;
    while (node) {
        if (node->frequency == c->frequency) {
            // Remove cache from the list
            if (node->head == c)
                node->head = c->next;
            if (c->prev)
                c->prev->next = c->next;
            if (c->next)
                c->next->prev = c->prev;

            c->next = c->prev = NULL;

            // Remove node if list is empty
            if (!node->head) {
                freq_node *curr = f->head, *prev = NULL;
                while (curr != node) {
                    prev = curr;
                    curr = curr->next;
                }
                if (prev)
                    prev->next = node->next;
                else
                    f->head = node->next;
                free(node);
            }
            return;
        }
        node = node->next;
    }
}

struct cache* freq_map_evict(freq_map* f) {
    if (!f || !f->head) return NULL;

    freq_node* node = f->head;
    struct cache* victim = node->head;
    struct cache* curr = victim;

    while (curr) {
        if (curr->time_track < victim->time_track)
            victim = curr;
        curr = curr->next;
    }

    freq_map_remove(f, victim);
    return victim;
}

void freq_map_free(freq_map* f) {
    if (!f) return;

    freq_node* node = f->head;
    while (node) {
        struct cache* curr = node->head;
        while (curr) {
            struct cache* tmp = curr;
            curr = curr->next;
            // Do not free cache unless you own it
        }
        freq_node* next = node->next;
        free(node);
        node = next;
    }
    free(f);
}
