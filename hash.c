#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

static void hm_resize(HashMap *hm, size_t new_capacity) {
    Bucket *old_buckets = hm->buckets;
    size_t old_capacity = hm->capacity;
    hm->buckets = calloc(new_capacity, sizeof(Bucket));
    hm->capacity = new_capacity;
    hm->count = 0;
    for(size_t i = 0; i < old_capacity; i++) {
        if(old_buckets[i].curr_state == SLOT_OCCUPIED) {
            hm_insert(hm, old_buckets[i].key, old_buckets[i].value);
            free(old_buckets[i].key);

        }
    }
    free(old_buckets);
}

HashMap *hm_create(void) { // create the hm
    HashMap *hm = malloc(sizeof(HashMap)); // allocate 1 header
    if(hm == NULL) { // malloc fail check
        return NULL; 
    }
    hm->buckets = calloc(CAPACITY, sizeof(*hm->buckets)); // allocate each bucket, calloc to 0 out
    if(hm->buckets == NULL) { // fail check, if it failed free hm and restart
        free(hm);
        return NULL;
    }
    hm->capacity = CAPACITY; // set params
    hm->count = 0;
    return hm; // return ptr
}

void hm_destroy(HashMap *hm) { // free the hm
    if(hm == NULL) {
        return;
    }
    for(size_t i = 0; i < hm->capacity; i++) {
        if(hm->buckets[i].curr_state == SLOT_OCCUPIED){
            free(hm->buckets[i].key);
        }
    }
    free(hm->buckets);
    free(hm);
}

static uint64_t hashfnv1a(const char *key) { // fnv1a hashing algo
    uint64_t hash = 0xcbf29ce484222325;
    const uint64_t fnv_prime = 0x100000001b3;
    while(*key != '\0') {
        hash = hash ^ (unsigned char)*key;
        hash = hash * fnv_prime;
        key++;
    }
    return hash;
}

void hm_insert(HashMap *hm, const char *key, int value) {
    if ((hm->count + 1) * 10 >= hm->capacity * 7) {
        hm_resize(hm, hm->capacity * 2);
    }

    uint64_t home_index = hashfnv1a(key) % hm->capacity; // hash % capacity to get valid index
    for(size_t i = 0; i < hm->capacity; i ++) { // walk thru 0 to capacity
        size_t slot = (home_index + i) & (hm->capacity - 1); 
        if(hm->buckets[slot].curr_state == SLOT_EMPTY) {
            char *usekey = strdup(key); // dupe so hm owns the key
            hm->buckets[slot].key = usekey;
            hm->buckets[slot].value = value;
            hm->buckets[slot].curr_state = SLOT_OCCUPIED;
            hm->count++;
            return;
        } else if(hm->buckets[slot].curr_state == SLOT_OCCUPIED) {
            if(strcmp(hm->buckets[slot].key, key) == 0) {
                hm->buckets[slot].value = value;
                return;
            }
        } else if(hm->buckets[slot].curr_state == SLOT_TOMBSTONE) {
            //fall thru to next iteration
        }
    }
}

bool hm_get(const HashMap *hm, const char *key, int *out) {
    uint64_t home_index = hashfnv1a(key) % hm->capacity; // hash % capacity to get valid index
    for(size_t i = 0; i < hm->capacity; i ++) { // walk thru 0 to capacity
        size_t slot = (home_index + i) & (hm->capacity - 1); 
        if(hm->buckets[slot].curr_state == SLOT_EMPTY) {
            return false;
        } else if(hm->buckets[slot].curr_state == SLOT_OCCUPIED) {
            if(strcmp(hm->buckets[slot].key, key) == 0) {
                *out = hm->buckets[slot].value;
                return true;
            }
        } else if(hm->buckets[slot].curr_state == SLOT_TOMBSTONE) {
            //fall thru to next iteration
        }
    }
    return false;
}    

bool hm_delete(HashMap *hm, const char *key) {
    uint64_t home_index = hashfnv1a(key) % hm->capacity; // hash % capacity to get valid index
    for(size_t i = 0; i < hm->capacity; i ++) { // walk thru 0 to capacity
        size_t slot = (home_index + i) & (hm->capacity - 1);  
        if(hm->buckets[slot].curr_state == SLOT_EMPTY) {
            return false;
        }
        else if(hm->buckets[slot].curr_state == SLOT_OCCUPIED && (strcmp(hm->buckets[slot].key, key) == 0)) {
            free(hm->buckets[slot].key);
            hm->buckets[slot].key = NULL;
            hm->buckets[slot].curr_state = SLOT_TOMBSTONE;
            hm->count--;
            return true;
        }
        else if(hm->buckets[slot].curr_state == SLOT_OCCUPIED && (strcmp(hm->buckets[slot].key, key) != 0)) {
            // fall thru
        }
        else if(hm->buckets[slot].curr_state == SLOT_TOMBSTONE){
            // fall thru
        }
    }
    return false;
}