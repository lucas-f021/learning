#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

typedef struct {
    void *start;
    void *nxt_byte;
    void *end;
} Arena;

Arena *arena_create(size_t size);
void arena_destroy(Arena *a);
void arena_reset(Arena *a);
void *arena_alloc(Arena *a, size_t size);


#endif