#ifndef HASH_H
#define HASH_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "value.h"

#define CAPACITY 16

typedef enum {
    SLOT_EMPTY,
    SLOT_OCCUPIED,
    SLOT_TOMBSTONE
} State;

typedef struct {
    State curr_state;
    char *key;
    Value value;
} Bucket;

typedef struct {
    Bucket *buckets;
    size_t capacity;
    size_t count;
} HashMap;

void hm_insert(HashMap *hm, const char *key, Value value);
HashMap *hm_create(void);
void hm_destroy(HashMap *hm);
bool hm_get(const HashMap *hm, const char *key, Value *out);
bool hm_delete(HashMap *hm, const char *key);


#endif