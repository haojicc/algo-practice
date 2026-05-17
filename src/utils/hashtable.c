/**
 * @file hashtable.c
 * @brief Lightweight hash table implementation used by ZSet.
 */

#include "hashtable.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t next_power_of_two(size_t value) {
    size_t result = 1;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

static HashEntry* create_entry(void* key, void* value) {
    HashEntry* entry = malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->key = key;
    entry->value = value;
    entry->next = NULL;
    return entry;
}

HashTable* hash_create(size_t initial_size,
                       size_t key_size,
                       HashFn hash,
                       KeyCompareFn compare) {
    if (initial_size == 0) {
        initial_size = 16;
    }
    HashTable* ht = calloc(1, sizeof(*ht));
    if (ht == NULL) {
        return NULL;
    }
    ht->size = next_power_of_two(initial_size);
    ht->used = 0;
    ht->key_size = key_size;
    ht->hash = hash != NULL ? hash : hash_default;
    ht->compare = compare != NULL ? compare : compare_default;
    ht->buckets = calloc(ht->size, sizeof(*ht->buckets));
    if (ht->buckets == NULL) {
        free(ht);
        return NULL;
    }
    return ht;
}

void hash_destroy(HashTable* ht) {
    if (ht == NULL) {
        return;
    }
    for (size_t index = 0; index < ht->size; ++index) {
        HashEntry* entry = ht->buckets[index];
        while (entry != NULL) {
            HashEntry* next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(ht->buckets);
    free(ht);
}

static bool hash_expand(HashTable* ht, size_t new_size) {
    if (new_size < 16) {
        new_size = 16;
    }
    new_size = next_power_of_two(new_size);
    HashEntry** buckets = calloc(new_size, sizeof(*buckets));
    if (buckets == NULL) {
        return false;
    }
    for (size_t index = 0; index < ht->size; ++index) {
        HashEntry* entry = ht->buckets[index];
        while (entry != NULL) {
            HashEntry* next = entry->next;
            uint64_t hash = ht->hash(entry->key, ht->key_size);
            size_t position = (size_t)(hash & (new_size - 1));
            entry->next = buckets[position];
            buckets[position] = entry;
            entry = next;
        }
    }
    free(ht->buckets);
    ht->buckets = buckets;
    ht->size = new_size;
    return true;
}

void* hash_put(HashTable* ht, void* key, void* value) {
    if (ht == NULL || key == NULL) {
        return NULL;
    }
    if (ht->used >= ht->size) {
        if (!hash_expand(ht, ht->size * 2)) {
            return NULL;
        }
    }
    uint64_t hash = ht->hash(key, ht->key_size);
    size_t index = (size_t)(hash & (ht->size - 1));
    HashEntry* entry = ht->buckets[index];
    while (entry != NULL) {
        if (ht->compare(entry->key, key, ht->key_size) == 0) {
            void* old_value = entry->value;
            entry->value = value;
            return old_value;
        }
        entry = entry->next;
    }
    HashEntry* next = create_entry(key, value);
    if (next == NULL) {
        return NULL;
    }
    next->next = ht->buckets[index];
    ht->buckets[index] = next;
    ht->used += 1;
    return NULL;
}

void* hash_get(const HashTable* ht, const void* key) {
    if (ht == NULL || key == NULL) {
        return NULL;
    }
    uint64_t hash = ht->hash(key, ht->key_size);
    size_t index = (size_t)(hash & (ht->size - 1));
    HashEntry* entry = ht->buckets[index];
    while (entry != NULL) {
        if (ht->compare(entry->key, key, ht->key_size) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

void* hash_remove(HashTable* ht, const void* key) {
    if (ht == NULL || key == NULL) {
        return NULL;
    }
    uint64_t hash = ht->hash(key, ht->key_size);
    size_t index = (size_t)(hash & (ht->size - 1));
    HashEntry* entry = ht->buckets[index];
    HashEntry* previous = NULL;
    while (entry != NULL) {
        if (ht->compare(entry->key, key, ht->key_size) == 0) {
            if (previous != NULL) {
                previous->next = entry->next;
            } else {
                ht->buckets[index] = entry->next;
            }
            void* value = entry->value;
            free(entry);
            ht->used -= 1;
            return value;
        }
        previous = entry;
        entry = entry->next;
    }
    return NULL;
}

bool hash_resize(HashTable* ht, size_t new_size) {
    if (ht == NULL || new_size < 16) {
        return false;
    }
    return hash_expand(ht, new_size);
}

uint64_t hash_default(const void* key, size_t key_size) {
    const unsigned char* data = (const unsigned char*)key;
    uint64_t hash = 1469598103934665603ULL;
    if (key_size == 0) {
        key_size = sizeof(void*);
    }
    for (size_t i = 0; i < key_size; ++i) {
        hash ^= (uint64_t)data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

int compare_default(const void* a, const void* b, size_t key_size) {
    if (a == b) {
        return 0;
    }
    if (key_size == 0) {
        return a < b ? -1 : 1;
    }
    int result = memcmp(a, b, key_size);
    return result;
}
