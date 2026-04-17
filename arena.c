#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "arena.h"

Arena *arena_create(size_t size) {
    Arena *new = malloc(sizeof(Arena));
    if(new == NULL) {
        return NULL;
    }
        
    new->start = malloc(size);
    if(new->start == NULL){
        free(new);
        return NULL;
    }

    new->nxt_byte = new->start;

    new->end = (uint8_t *)new->start + size;

    return new;
}

void arena_destroy(Arena *a) {
    if(a == NULL) {
        return;
    }
    free(a->start);
    free(a);
}

void arena_reset(Arena *a) {
    a->nxt_byte = a->start;
}

void *arena_alloc(Arena *a, size_t size) {
    uintptr_t n = (uintptr_t)a->nxt_byte;
    uintptr_t algn = (n + 7) & ~(uintptr_t)7;

    if((uintptr_t)(algn + size) > (uintptr_t)(a->end)) {
        return NULL;
    }

    a->nxt_byte = (void *)(algn+size);

    return (void *)algn;
}

