#ifndef HASHMAP_H
#define HASHMAP_H

#include "pico/stdlib.h"

typedef struct HashNode {
    uint16_t key;
    uint16_t value;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode **buckets;
    uint16_t size;
    uint16_t count;
} HashMap;

HashMap* create_hash_map();
void delete_hash_map(HashMap *map);
void reset_hash_map(HashMap *map);
bool put_in_map(HashMap *map, uint16_t key, uint16_t value);
void remove_multiple_by_value(HashMap *map, uint16_t start_value, uint16_t end_value);
bool get_from_map(HashMap *map, uint16_t key, uint16_t *value);
bool is_key_in_map(HashMap *map, uint16_t key);
uint16_t get_key_from_value(HashMap *map, uint16_t value);
uint32_t get_hash_map_memory_usage(HashMap *map);

#endif
