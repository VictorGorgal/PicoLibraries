// hashset.c

#include "hashmap.h"
#include <stdlib.h>
#include "pico/stdlib.h"

#define INITIAL_SIZE 2

static unsigned int hash(uint16_t key, uint16_t size) {
    return key % size;
}

static void resize(HashMap *map) {
    uint16_t new_size = map->size * 2;
    HashNode **new_buckets = calloc(new_size, sizeof(HashNode *));
    
    if (!new_buckets) return; // Handle allocation failure

    // Rehash existing nodes
    for (uint16_t i = 0; i < map->size; ++i) {
        HashNode *node = map->buckets[i];
        while (node) {
            HashNode *next = node->next;
            unsigned int new_index = hash(node->key, new_size);
            node->next = new_buckets[new_index];
            new_buckets[new_index] = node;
            node = next;
        }
    }

    free(map->buckets);
    map->buckets = new_buckets;
    map->size = new_size;
}

HashMap* create_hash_map() {
    HashMap *map = malloc(sizeof(HashMap));
    if (!map) return NULL;

    map->buckets = calloc(INITIAL_SIZE, sizeof(HashNode *));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    map->size = INITIAL_SIZE;
    map->count = 0; // Initialize count
    return map;
}

void delete_hash_map(HashMap *map) {
    if (!map) return;

    for (uint16_t i = 0; i < map->size; ++i) {
        HashNode *node = map->buckets[i];
        while (node) {
            HashNode *temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(map->buckets);
    free(map);
}

void reset_hash_map(HashMap *map) {
    if (!map) return;

    // Free all nodes in the current buckets
    for (uint16_t i = 0; i < map->size; ++i) {
        HashNode *node = map->buckets[i];
        while (node) {
            HashNode *temp = node;
            node = node->next;
            free(temp);
        }
    }

    // Free the current buckets
    free(map->buckets);

    // Allocate new buckets with the original size
    map->buckets = calloc(INITIAL_SIZE, sizeof(HashNode *));
    if (!map->buckets) {
        // Handle allocation failure
        map->size = 0; // Set size to 0 on failure
        return;
    }

    map->size = INITIAL_SIZE;
    map->count = 0; // Reset count
}

bool put_in_map(HashMap *map, uint16_t key, uint16_t value) {
    if (!map) return false;

    // Resize if necessary (load factor threshold, e.g., 0.75)
    if ((double)map->count / map->size > 0.75) {
        resize(map);
    }

    unsigned int index = hash(key, map->size);
    HashNode *node = map->buckets[index];

    // Check if the key is already in the map
    while (node) {
        if (node->key == key) {
            node->value = value; // Update value
            return true;
        }
        node = node->next;
    }

    // Create a new node
    HashNode *new_node = malloc(sizeof(HashNode));
    if (!new_node) return false;

    new_node->key = key;
    new_node->value = value;
    new_node->next = map->buckets[index];
    map->buckets[index] = new_node;
    map->count++; // Increment count
    return true;
}

void remove_multiple_by_value(HashMap *map, uint16_t start_value, uint16_t end_value) {
    if (!map) return;

    for (uint16_t i = 0; i < map->size; ++i) {
        HashNode *node = map->buckets[i];
        HashNode *prev = NULL;

        while (node) {
            if (node->value >= start_value && node->value <= end_value) {
                // Found a node to remove
                if (prev) {
                    prev->next = node->next; // Link previous to next
                } else {
                    map->buckets[i] = node->next; // Move head
                }
                HashNode *temp = node;
                node = node->next; // Move to next node
                free(temp); // Free the removed node
                map->count--; // Decrement count
            } else {
                prev = node; // Move to the next node
                node = node->next;
            }
        }
    }
}


bool get_from_map(HashMap *map, uint16_t key, uint16_t *value) {
    if (!map) return false;

    unsigned int index = hash(key, map->size);
    HashNode *node = map->buckets[index];

    while (node) {
        if (node->key == key) {
            *value = node->value;
            return true; // Found
        }
        node = node->next;
    }
    return false; // Not found
}

bool is_key_in_map(HashMap *map, uint16_t key) {
    if (!map) return false;

    unsigned int index = hash(key, map->size);
    HashNode *node = map->buckets[index];

    while (node) {
        if (node->key == key) {
            return true; // Key found
        }
        node = node->next;
    }
    return false; // Key not found
}

uint16_t get_key_from_value(HashMap *map, uint16_t value) {
    if (!map) return false;

    for (uint16_t i = 0; i < map->size; ++i) {
        HashNode *node = map->buckets[i];
        while (node) {
            if (node->value == value) {
                return node->key;
            }
            node = node->next;
        }
    }
}

uint32_t get_hash_map_memory_usage(HashMap *map) {
    if (!map) return 0;

    uint32_t total_memory = sizeof(HashMap); // Memory for the hashmap structure
    total_memory += map->size * sizeof(HashNode *); // Memory for the buckets

    // Iterate through each bucket to calculate memory for nodes
    for (uint16_t i = 0; i < map->size; ++i) {
        HashNode *node = map->buckets[i];
        while (node) {
            total_memory += sizeof(HashNode); // Memory for each node
            node = node->next; // Move to the next node
        }
    }

    return total_memory;
}
